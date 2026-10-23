#!/usr/bin/env python3

import logging
import os
import shutil
from .gen_ppc import *
from .gen_otp import *
from .bl1_sign import bl1_sign, bl1_sign_hash
from .bl2_sign import bl2_sign, bl2_sign_hash
from .compress import *
from .common import check_signing_key_pair
from .image_check import Reporter, check_images
from .pk_hash import insert_pk_hash
from .save_cmd import reset_cmd_list
from .pipeline import Action, PackContext, register, run_pipeline

def pack_all(config_dir, aes_key, preset='all'):
    """Run the "all" pipeline declared in pack.json (default: the legacy flow)."""
    ctx = PackContext(config_dir, aes_key)
    ctx.chdir()
    # Truncate pack_cmd_list.txt once per run, before any action records a command.
    reset_cmd_list()
    run_pipeline(ctx.steps(preset), ctx)

def pack_run(preset_or_steps, config_dir=None, aes_key=None):
    """Run one pipeline preset (or a "A,B,C" action list) declared in pack.json.

    Unlike pack_all() this does not reset pack_cmd_list.txt: a split-signing
    build runs several stages in a row and the log must keep them all.
    """
    ctx = PackContext(config_dir, aes_key)
    ctx.chdir()
    run_pipeline(ctx.steps(preset_or_steps), ctx)

@register
class GenPpc(Action):
    """Generate the PPC (peripheral permission) header."""

    name = 'GEN_PPC'

    def run(self):
        gen_ppc_config_file('ppc.csv', 'gpio_dev.csv', '_ppc.h')

@register
class CheckKeys(Action):
    """Fail early when the signing key pair does not match (secureboot only)."""

    name = 'CHECK_KEYS'

    def run(self):
        s = self.ctx.security
        if s.secureboot_en:
            check_signing_key_pair(s.bl1_root_privkey, s.bl1_root_pubkey, 'pack_all: ')

@register
class GenOtpEfuse(Action):
    """Generate otp_efuse_config.json (ROTPK hashes + flash AES key)."""

    name = 'GEN_OTP_EFUSE'

    def run(self):
        s = self.ctx.security
        o = self.ctx.ota
        gen_otp_efuse_config_file(s.flash_aes_type, self.ctx.otp_flash_aes_key,
                                  s.bl2_root_pubkey, s.secureboot_en,
                                  o.get_boot_ota(), 'otp_efuse_config.json')

@register
class PrepareBins(Action):
    """Build the unsigned inputs consumed by the signing steps."""

    name = 'PREPARE_BINS'

    def run(self):
        self.ctx.partitions.gen_bins_for_bl2_signing()

@register
class SignManifest(Action):
    """Sign the BL1 manifest with the BL1 root key."""

    name = 'SIGN_MANIFEST'

    def run(self):
        pbl2 = self.bl2_partition()
        if pbl2 == None:
            return

        s = self.ctx.security
        p = self.ctx.partitions
        bl1_sign('sign', s.bl1_root_key_type, s.bl1_root_privkey, s.bl1_root_pubkey, None, pbl2.bin_name, pbl2.load_addr, pbl2.static_addr, 'primary_manifest.bin')
        if (self.ctx.boot_ota == True):
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

@register
class SignApp(Action):
    """Sign the primary_all image with the BL2 root key."""

    name = 'SIGN_APP'

    def run(self):
        pbl2 = self.bl2_partition()
        if pbl2 == None:
            return

        s = self.ctx.security
        pall = self.ctx.partitions.find_partition_by_name('primary_all')
        # Base sign of primary_all. No --pad (swap trailer unused on BK7259) keeps
        # the image content-sized, shrinking all_app.bin. OVERWRITE re-signs it
        # below, so this only affects the XIP slot-A image.
        bl2_sign('sign', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, None, 'primary_all_code.bin', pall.vir_sign_size, '0.0.1', self.ctx.ota.get_app_security_counter(), 'primary_all_code_signed.bin', 'app_hash.json', pad=False)

@register
class SignAppForOta(Action):
    """OVERWRITE only: re-sign primary_all with the app version from ota.csv."""

    name = 'SIGN_APP_FOR_OTA'

    def run(self):
        if self.ctx.ota_type != 'OVERWRITE':
            return

        pbl2 = self.bl2_partition()
        if pbl2 == None:
            return

        s = self.ctx.security
        o = self.ctx.ota
        pall = self.ctx.partitions.find_partition_by_name('primary_all')
        bl2_sign('sign', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, None, 'primary_all_code.bin', pall.vir_sign_size, o.get_version(), o.get_app_security_counter(), 'primary_all_code_signed.bin', 'app_hash.json', pad=False)

@register
class CompressApp(Action):
    """OVERWRITE only: compress the signed app image for the OTA payload."""

    name = 'COMPRESS_APP'

    def run(self):
        if self.ctx.ota_type != 'OVERWRITE':
            return

        pbl2 = self.bl2_partition()
        if pbl2 == None:
            return

        if self.ctx.partitions.find_partition_by_name('ota') is None:
            # SIGN_OTA reports the skip; nothing to compress without an ota slot.
            return

        compress_bin('primary_all_code_signed.bin', 'compress.bin')

@register
class SignOta(Action):
    """Sign the OTA payload: the compressed image (OVERWRITE) or the app image (XIP)."""

    name = 'SIGN_OTA'

    def run(self):
        pbl2 = self.bl2_partition()
        if pbl2 == None:
            return

        s = self.ctx.security
        o = self.ctx.ota
        p = self.ctx.partitions
        ota_type = self.ctx.ota_type

        if (ota_type == 'OVERWRITE'):
            pota = p.find_partition_by_name('ota')
            if pota is None:
                logging.debug('OVERWRITE: no ota partition, skip compress/sign ota.bin')
                return
            # Compressed ota image: no --pad (see bl2_sign) -> ota.bin stays as
            # small as the compressed payload instead of the whole ota partition.
            bl2_sign('sign', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, None, 'compress.bin', pota.partition_size, o.get_version(), o.get_app_security_counter(), 'ota_signed.bin', 'ota_hash.json', pad=False)
        elif (ota_type == 'XIP'):
            pall = p.find_partition_by_name('primary_all')
            # No --pad: the mcuboot swap trailer (magic/copy_done/image_ok) is not
            # consumed on BK7259 (DIRECT_XIP_REVERT off; slot picked by header
            # version + boot_param A/B), so pad the XIP ota image only up to its
            # real content instead of the whole primary_all slot.
            bl2_sign('sign', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, None, 'primary_all_code.bin', pall.vir_sign_size, o.get_version(), o.get_app_security_counter(), 'ota_signed.bin', 'ota_hash.json', pad=False)

@register
class PackContainers(Action):
    """Encrypt/CRC the partitions and merge them into all-app.bin / ota.bin / bootloader.bin."""

    name = 'PACK_CONTAINERS'

    def run(self):
        s = self.ctx.security
        o = self.ctx.ota
        self.ctx.partitions.pack_bin('pack.json', s.flash_aes_type, self.ctx.flash_aes_key,
                                     o.get_app_security_counter(), o.get_encrypt(), o.get_boot_ota())

@register
class Install(Action):
    """Collect the produced artifacts into install/."""

    name = 'INSTALL'

    def run(self):
        self.ctx.partitions.install_bin()

@register
class ImageCheck(Action):
    """Self-check the produced artifacts (container structure + CRCs, no keys)."""

    name = 'IMAGE_CHECK'

    def run(self):
        rep = Reporter()
        # ota.bin is only produced when the config has an ota partition.
        if not check_images('all-app.bin',
                            'ota.bin' if os.path.isfile('ota.bin') else None,
                            'bootloader.bin', rep, crc_en=self.ctx.crc_en):
            logging.error('image self-check failed, see the report above')
            exit(1)

# ---------------------------------------------------------------------------
# Split signing (S3): the digest is computed on the pack server, only the
# digest travels to the sign server, and the returned signature is applied back
# on the pack server. The preset pipelines live in pipeline.py; every hash step
# here must pass exactly the same (version, security_counter) as its apply step,
# otherwise the signed image fails verification on the device.
# ---------------------------------------------------------------------------

def _read_json_field(json_file, field):
    """Read a single field of a hash/signature JSON produced by the tools."""
    with open(json_file, 'r') as f:
        return json.load(f)[field]


def _write_bl1_signature(sig_json='manifest_sig.json'):
    """Restore bl1_signature.txt from a manifest signature JSON.

    `secure_boot_tool gen_manifest_with_signature` has no signature option: it
    always reads the fixed-name file bl1_signature.txt, which `bl1_sign_hash()`
    turned into JSON. Line order is load-bearing -- line 1 = bl1_sig_s, line 2 =
    bl1_sig_r, exactly the order bl1_sign_hash() reads them back.
    """
    with open(sig_json, 'r') as f:
        sig = json.load(f)
    with open('bl1_signature.txt', 'w') as f:
        f.write(sig['bl1_sig_s'].strip())
        f.write('\n')
        f.write(sig['bl1_sig_r'].strip())


def _app_sign_version(ctx):
    """Version of the app image, shared by HASH_APP and APPLY_SIG_APP.

    Both halves of the split signing must hash the same image, so the rule lives
    here instead of being spelled out twice: XIP uses the fixed '0.0.1' like
    SIGN_APP, OVERWRITE uses ota.csv's app_version like SIGN_APP_FOR_OTA (that
    is the value surviving into the packed image).
    """
    if ctx.ota_type == 'XIP':
        return '0.0.1'
    return ctx.ota.get_version()


@register
class CheckConfig(Action):
    """Fail fast when the inputs of the hash/pack steps are missing."""

    name = 'CHECK_CONFIG'

    def run(self):
        for f in ('bl2.bin', 'tfm_s.bin', 'cpu0_app.bin',
                  'security.csv', 'ota.csv', 'partitions.csv'):
            if not os.path.exists(f):
                logging.error(f'{f} NOT exists')
                exit(1)


@register
class HashManifest(Action):
    """Compute the BL1 manifest digest(s) instead of signing them."""

    name = 'HASH_MANIFEST'

    def run(self):
        pbl2 = self.bl2_partition()
        if pbl2 == None:
            return

        s = self.ctx.security
        p = self.ctx.partitions
        pbl2_B = p.find_partition_by_name('bl2_B')
        if (self.ctx.boot_ota == True):
            # bl1_sign('hash') always writes manifest_hash.json, so the
            # secondary digest has to be produced first and renamed.
            bl1_sign('hash', s.bl1_root_key_type, s.bl1_root_privkey, s.bl1_root_pubkey, None, pbl2_B.bin_name, pbl2_B.load_addr, pbl2_B.static_addr, 'secondary_manifest.bin')
            shutil.move('manifest_hash.json', 'secondary_manifest_hash.json')
        bl1_sign('hash', s.bl1_root_key_type, s.bl1_root_privkey, s.bl1_root_pubkey, None, pbl2.bin_name, pbl2.load_addr, pbl2.static_addr, 'primary_manifest.bin')
        if (self.ctx.boot_ota == False):
            if os.path.isfile('primary_manifest.bin'):
                shutil.copy2('primary_manifest.bin', 'secondary_manifest.bin')
                logging.debug('boot_ota disabled: copied primary_manifest.bin -> secondary_manifest.bin')
            if pbl2_B is not None and os.path.isfile(pbl2.bin_name) and not os.path.isfile(pbl2_B.bin_name):
                shutil.copy2(pbl2.bin_name, pbl2_B.bin_name)
                logging.debug('boot_ota disabled: copied %s -> %s' % (pbl2.bin_name, pbl2_B.bin_name))


@register
class HashApp(Action):
    """Compute the digest of the app image (pack server side)."""

    name = 'HASH_APP'

    def run(self):
        pbl2 = self.bl2_partition()
        if pbl2 == None:
            return

        s = self.ctx.security
        pall = self.ctx.partitions.find_partition_by_name('primary_all')
        bl2_sign('hash', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, None, 'primary_all_code.bin', pall.vir_sign_size, _app_sign_version(self.ctx), self.ctx.ota.get_app_security_counter(), 'primary_all_code_signed.bin', 'app_hash.json', pad=False)


@register
class HashOta(Action):
    """Compute the digest of the OTA payload (pack server side).

    OVERWRITE: the payload is the compressed *signed* app image, so this step
    must run after the app signature has been applied; COMPRESS_APP produces
    compress.bin from that image.
    XIP: the payload is a freshly signed copy of primary_all_code.bin and is
    independent of the app signing steps.
    """

    name = 'HASH_OTA'

    def run(self):
        pbl2 = self.bl2_partition()
        if pbl2 == None:
            return

        s = self.ctx.security
        o = self.ctx.ota
        p = self.ctx.partitions
        ota_type = self.ctx.ota_type

        if (ota_type == 'OVERWRITE'):
            pota = p.find_partition_by_name('ota')
            if pota is None:
                logging.debug('OVERWRITE: no ota partition, skip ota hash')
                return
            bl2_sign('hash', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, None, 'compress.bin', pota.partition_size, o.get_version(), o.get_app_security_counter(), 'ota_signed.bin', 'ota_hash.json', pad=False)
        elif (ota_type == 'XIP'):
            pall = p.find_partition_by_name('primary_all')
            bl2_sign('hash', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, None, 'primary_all_code.bin', pall.vir_sign_size, o.get_version(), o.get_app_security_counter(), 'ota_signed.bin', 'ota_hash.json', pad=False)


@register
class SignHashManifest(Action):
    """Turn the manifest digest(s) into signature JSON (sign server side)."""

    name = 'SIGN_HASH_MANIFEST'

    def run(self):
        s = self.ctx.security
        bl1_sign_hash(s.bl1_root_privkey, _read_json_field('manifest_hash.json', 'hash'), 'manifest_sig.json')
        if (self.ctx.boot_ota == True):
            bl1_sign_hash(s.bl1_root_privkey, _read_json_field('secondary_manifest_hash.json', 'hash'), 'secondary_manifest_sig.json')


@register
class SignHashApp(Action):
    """Turn the app digest into a signature JSON (sign server side)."""

    name = 'SIGN_HASH_APP'

    def run(self):
        s = self.ctx.security
        bl2_sign_hash(s.bl2_root_privkey, _read_json_field('app_hash.json', 'hash'), 'app_sig.json')


@register
class SignHashOta(Action):
    """Turn the OTA digest into a signature JSON (sign server side)."""

    name = 'SIGN_HASH_OTA'

    def run(self):
        if not os.path.isfile('ota_hash.json'):
            logging.debug('no ota_hash.json, skip ota signature')
            return
        s = self.ctx.security
        bl2_sign_hash(s.bl2_root_privkey, _read_json_field('ota_hash.json', 'hash'), 'ota_sig.json')


@register
class ApplySigManifest(Action):
    """Rebuild primary/secondary_manifest.bin from the manifest signature."""

    name = 'APPLY_SIG_MANIFEST'

    def run(self):
        pbl2 = self.bl2_partition()
        if pbl2 == None:
            return

        s = self.ctx.security
        p = self.ctx.partitions
        _write_bl1_signature('manifest_sig.json')
        bl1_sign('sign_from_sig', s.bl1_root_key_type, s.bl1_root_privkey, s.bl1_root_pubkey, None, pbl2.bin_name, pbl2.load_addr, pbl2.static_addr, 'primary_manifest.bin')
        if (self.ctx.boot_ota == True):
            pbl2_B = p.find_partition_by_name('bl2_B')
            _write_bl1_signature('secondary_manifest_sig.json')
            bl1_sign('sign_from_sig', s.bl1_root_key_type, s.bl1_root_privkey, s.bl1_root_pubkey, None, pbl2_B.bin_name, pbl2_B.load_addr, pbl2_B.static_addr, 'secondary_manifest.bin')
        else:
            if os.path.isfile('primary_manifest.bin'):
                shutil.copy2('primary_manifest.bin', 'secondary_manifest.bin')
                logging.debug('boot_ota disabled: copied primary_manifest.bin -> secondary_manifest.bin')
            pbl2_B = p.find_partition_by_name('bl2_B')
            if pbl2_B is not None and os.path.isfile(pbl2.bin_name) and not os.path.isfile(pbl2_B.bin_name):
                shutil.copy2(pbl2.bin_name, pbl2_B.bin_name)
                logging.debug('boot_ota disabled: copied %s -> %s' % (pbl2.bin_name, pbl2_B.bin_name))


@register
class ApplySigApp(Action):
    """Rebuild the signed app image from the app signature."""

    name = 'APPLY_SIG_APP'

    def run(self):
        pbl2 = self.bl2_partition()
        if pbl2 == None:
            return

        s = self.ctx.security
        pall = self.ctx.partitions.find_partition_by_name('primary_all')
        app_sig = _read_json_field('app_sig.json', 'signature')
        bl2_sign('sign_from_sig', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, app_sig, 'primary_all_code.bin', pall.vir_sign_size, _app_sign_version(self.ctx), self.ctx.ota.get_app_security_counter(), 'primary_all_code_signed.bin', 'app_hash.json', pad=False)


@register
class ApplySigOta(Action):
    """Rebuild ota_signed.bin from the OTA signature."""

    name = 'APPLY_SIG_OTA'

    def run(self):
        pbl2 = self.bl2_partition()
        if pbl2 == None:
            return

        ota_type = self.ctx.ota_type
        if (self.ctx.partitions.find_partition_by_name('ota') is None) and (ota_type == 'OVERWRITE'):
            logging.debug('OVERWRITE: no ota partition, skip ota sign_from_sig')
            return
        if not os.path.isfile('ota_sig.json'):
            logging.debug('no ota_sig.json, skip ota sign_from_sig')
            return

        s = self.ctx.security
        o = self.ctx.ota
        p = self.ctx.partitions
        ota_sig = _read_json_field('ota_sig.json', 'signature')

        if (ota_type == 'OVERWRITE'):
            pota = p.find_partition_by_name('ota')
            bl2_sign('sign_from_sig', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, ota_sig, 'compress.bin', pota.partition_size, o.get_version(), o.get_app_security_counter(), 'ota_signed.bin', 'ota_hash.json', pad=False)
        elif (ota_type == 'XIP'):
            pall = p.find_partition_by_name('primary_all')
            bl2_sign('sign_from_sig', s.bl2_root_key_type, s.bl2_root_privkey, s.bl2_root_pubkey, ota_sig, 'primary_all_code.bin', pall.vir_sign_size, o.get_version(), o.get_app_security_counter(), 'ota_signed.bin', 'ota_hash.json', pad=False)


@register
class InsertPkHash(Action):
    """Mirror the ROTPK hashes into the bootloader image (flash 0xFC0..0x1000).

    Legacy step carried over from bksecure: it overwrites the last 64 bytes of
    the first 4K block with [bl1_rotpk_hash||bl2_rotpk_hash] (see
    scripts/pk_hash.py). Deliberately NOT part of any built-in preset:

      * bk7259's boot chain takes both hashes from OTP eFuse
        (gen_otp_config.yaml: bl1_rotpk_hash@0x42100628,
        bl2_rotpk_hash@0x42100648) and no in-tree reader looks at this slot --
        TF-M uses tfm_plat_otp_read(PLAT_OTP_ID_BL2_ROTPK_0);
      * the slot currently ships as 0xFF and the artifact set verifies and
        boots without it, so enabling it by default would change
        bootloader.bin without a known consumer.

    Enable it per project through pack.json "pipelines" (or `pack run --steps`)
    when the ROM is expected to read the in-image copy. Order matters: after
    PACK_CONTAINERS (which produces bootloader.bin) and before INSTALL, else the
    install/ copy keeps the unpatched bytes. Only the public key is needed, so
    the pack server of the split-signing flow stays keyless.
    """

    name = 'INSERT_PK_HASH'

    def run(self):
        if self.ctx.security.secureboot_en == False:
            logging.debug('INSERT_PK_HASH: secureboot disabled, skipped')
            return
        if not os.path.isfile('bootloader.bin'):
            logging.debug('INSERT_PK_HASH: no bootloader.bin, skipped')
            return
        insert_pk_hash('bootloader.bin', self.ctx.security.bl2_root_pubkey)
