#!/usr/bin/env python3

import logging
import os
import shutil
from .gen_ppc import *
from .gen_mpc import *
from .gen_security import *
from .gen_ota import *
from .gen_otp import *
from .bl1_sign import bl1_sign
from .bl2_sign import bl2_sign
from scripts.partition import *
from .compress import *
from .gen_ppc import *

def pack_all(config_dir, aes_key):
    if config_dir != None:
        logging.debug(f'cd {config_dir}')
        os.chdir(config_dir)

    gen_ppc_config_file('ppc.csv', 'gpio_dev.csv', '_ppc.h')

    o = OTA('ota.csv')
    s = Security('security.csv')

    if aes_key != None:
        flash_aes_key = aes_key
        otp_flash_aes_key = aes_key
    else:
        flash_aes_key = s.flash_aes_key 
        otp_flash_aes_key = s.raw_flash_aes_key

    gen_otp_efuse_config_file(s.flash_aes_type, otp_flash_aes_key, s.bl2_root_pubkey, s.secureboot_en, o.get_boot_ota(),'otp_efuse_config.json')

    ota_type = o.get_strategy()

    boot_ota = o.get_boot_ota()

    p = Partitions('partitions.csv', ota_type, boot_ota, s.secureboot_en, s.crc_en, s.sig_verify_en)

    p.gen_bins_for_bl2_signing()

    pbl2 = p.find_partition_by_name('bl2')
    if pbl2 != None:
        bl1_sign('sign', s.bl1_root_key_type, s.bl1_root_privkey, s.bl1_root_pubkey, None, pbl2.bin_name, pbl2.load_addr, pbl2.static_addr, 'primary_manifest.bin')
        if (boot_ota == True):
            pbl2_B = p.find_partition_by_name('bl2_B')
            bl1_sign('sign', s.bl1_root_key_type, s.bl1_root_privkey, s.bl1_root_pubkey, None, pbl2_B.bin_name, pbl2_B.load_addr, pbl2_B.static_addr, 'secondary_manifest.bin')
        else:
            if os.path.isfile('primary_manifest.bin'):
                shutil.copy2('primary_manifest.bin', 'secondary_manifest.bin')
                logging.debug('boot_ota disabled: copied primary_manifest.bin -> secondary_manifest.bin')
            pbl2_B = p.find_partition_by_name('bl2_B')
            if pbl2_B is not None and os.path.isfile(pbl2.bin_name) and not os.path.isfile(pbl2_B.bin_name):
                shutil.copy2(pbl2.bin_name, pbl2_B.bin_name)
                logging.debug('boot_ota disabled: copied %s -> %s' % (pbl2.bin_name, pbl2_B.bin_name))

        pall = p.find_partition_by_name('primary_all')
        # Base sign of primary_all. No --pad (swap trailer unused on BK7259) keeps
        # the image content-sized, shrinking all_app.bin. OVERWRITE re-signs it
        # below, so this only affects the XIP slot-A image.
        bl2_sign('sign', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, None, 'primary_all_code.bin', pall.vir_sign_size, '0.0.1', o.get_app_security_counter(), 'primary_all_code_signed.bin', 'app_hash.json', pad=False)

        if (ota_type == 'OVERWRITE'):
            app_version = o.get_version()
            bl2_sign('sign', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, None, 'primary_all_code.bin', pall.vir_sign_size, app_version, o.get_app_security_counter(), 'primary_all_code_signed.bin', 'app_hash.json', pad=False)
            compress_bin('primary_all_code_signed.bin', 'compress.bin')
            pota = p.find_partition_by_name('ota')
            # Compressed ota image: no --pad (see bl2_sign) -> ota.bin stays as
            # small as the compressed payload instead of the whole ota partition.
            bl2_sign('sign', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, None, 'compress.bin', pota.partition_size, app_version, o.get_app_security_counter(), 'ota_signed.bin', 'ota_hash.json', pad=False)
        elif (ota_type == 'XIP'):
            pota = p.find_partition_by_name('primary_all')
            app_version = o.get_version()
            # No --pad: the mcuboot swap trailer (magic/copy_done/image_ok) is not
            # consumed on BK7259 (DIRECT_XIP_REVERT off; slot picked by header
            # version + boot_param A/B), so pad the XIP ota image only up to its
            # real content instead of the whole primary_all slot.
            bl2_sign('sign', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, None, 'primary_all_code.bin', pall.vir_sign_size, app_version, o.get_app_security_counter(), 'ota_signed.bin', 'ota_hash.json', pad=False)

    p.pack_bin('pack.json', s.flash_aes_type, flash_aes_key, o.get_app_security_counter(),o.get_encrypt(), o.get_boot_ota())
    p.install_bin()
