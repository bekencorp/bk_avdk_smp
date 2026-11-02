#!/usr/bin/env python3

import os
import json
import logging
from .common import *
from .genbl1 import *
from .bl_stamp import write_stamp, BL2_INTERNAL_VERSION_STAMP_MAGIC
from .ota import OTA, BL2_VERSION_STAMP_OFFSET, BL2_INTERNAL_VERSION_STAMP_OFFSET
from .save_cmd import save_cmd

# Env var carrying the internal SDK BL2 version, set by tools/build_main.mk.
BL2_INTERNAL_VERSION_ENV = 'RECORD_BL2_INTERNAL_VERSION'

def bl1_sign_hash(privkey_pem_file, manifest_hash, outfile):
    sh_sec_tools = get_secure_boot_tool_exe()

    digest_file = '_bl1_manifest_digest.txt'
    with open(digest_file, 'w') as f:
        f.write(manifest_hash)

    cmd = f'{sh_sec_tools} sign -k {privkey_pem_file} -d {digest_file}'
    save_cmd('BL1 sign manifest hash', cmd)
    run_cmd(cmd)

    signature_dict = {
        'bl1_sig_s': '',
        'bl1_sig_r': '',
    }

    with open('bl1_signature.txt', 'r') as f:
        signature_dict['bl1_sig_s'] = f.readline().strip();
        signature_dict['bl1_sig_r'] = f.readline();

    with open(outfile, 'w') as f:
        json.dump(signature_dict, f, indent=4)

def gen_manifest_bin(action_type, manifest_json_file, outfile):
    sh_sec_tools = get_secure_boot_tool_exe()
    pwd = os.getcwd()
    logging.debug(f'generate manifest.bin')

    if (action_type == 'hash'):
        logging.debug(f'generate manifest hash only')
        cmd = f'{sh_sec_tools} manifest_digest -k key_desc.json -m {manifest_json_file} -o {pwd}/'
    elif (action_type == 'sign_from_sig'):
        logging.debug(f'generate manifest.bin from signature')
        cmd = f'{sh_sec_tools} gen_manifest_with_signature -k key_desc.json -m {manifest_json_file} -o {pwd}/'
    elif (action_type == 'sign'):
        logging.debug(f'generate manifest.bin from private key')
        cmd = f'{sh_sec_tools} -k key_desc.json -m {manifest_json_file} -o {pwd}/'
    else:
        logging.error(f'invalid bl1_sign action {action_type}')
        exit(1)

    save_cmd(f'BL1 manifest ({action_type})', cmd)
    run_cmd(cmd)
    move_file('manifest.bin', outfile)

    if (action_type == 'hash'):
        data = {"hash":""}
        with open('bl1_manifest_digest.txt', 'r') as f:
            h = f.read()
            data['hash'] = h
            with open('manifest_hash.json', 'w') as hf:
                json.dump(data, hf, indent=4)
        os.remove('bl1_manifest_digest.txt')

def load_ota_csv():
    # ota.csv is staged into the packaging dir; absent in projects without OTA.
    if os.path.exists('ota.csv'):
        return OTA('ota.csv')
    return None

def write_bl2_version_stamps(outfile):
    """Stamp the internal SDK BL2 version at 0xFE0 and, when ota.csv supplies
    bootloader_ota_version, the customer version at 0xFF0. Both land in the 0xFF
    padding tail of the manifest slot, so the signed payload is untouched. The
    customer stamp is skipped (with a warning) when the field is absent."""
    internal_version = os.environ.get(BL2_INTERNAL_VERSION_ENV, '').strip()
    if not internal_version:
        # The internal stamp is mandatory: a build that bypasses build_main.mk is
        # a misconfiguration.
        logging.error(f'{BL2_INTERNAL_VERSION_ENV} is not set, '
                      f'cannot stamp the internal BL2 version of {outfile}')
        exit(1)
    write_stamp(outfile, BL2_INTERNAL_VERSION_STAMP_OFFSET, internal_version,
                BL2_INTERNAL_VERSION_STAMP_MAGIC)

    ota = load_ota_csv()
    # OTA subclasses list without adding items, so bool(ota) is always False.
    version_str = ota.get_bootloader_ota_version() if ota is not None else None
    if version_str:
        write_stamp(outfile, BL2_VERSION_STAMP_OFFSET, version_str)
        logging.debug(f'bl2 version stamp of {outfile}: internal {internal_version}, '
                      f'customer {version_str}')
    else:
        # Optional: an erased slot reads back as "absent".
        logging.warning(f'no bootloader_ota_version, skip the customer BL2 stamp '
                        f'of {outfile}')

def bl1_sign(action_type, key_type, privkey_pem_file, pubkey_pem_file, signature, bin_file, static_addr, load_addr, outfile):
    if outfile == "secondary_manifest.bin" :
        manifest_json_file = 'secondary_manifest.json'
    else :
        manifest_json_file = 'primary_manifest.json'
    if action_type in ('hash', 'sign_from_sig'):
        # Neither digesting nor re-assembling a manifest needs the private key,
        # and key_desc.json carries mnft_prvkey verbatim: secure_boot_tool fails
        # if that path does not exist. Blanking it here keeps the pack server of
        # the split-signing flow free of any private key material.
        privkey_pem_file = ''
    g = Genbl1(True, key_type, privkey_pem_file, pubkey_pem_file, outfile)
    g.gen_key_desc()
    g.gen_manifest(7, static_addr, load_addr, bin_file, manifest_json_file)
    gen_manifest_bin(action_type, manifest_json_file, outfile)
    write_bl2_version_stamps(outfile)
