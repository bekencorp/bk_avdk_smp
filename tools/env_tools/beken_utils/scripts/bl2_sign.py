#!/usr/bin/env python3

import os
import json
import logging
from .common import *
from .genbl1 import *
from .save_cmd import save_cmd

def bl2_sign_hash(privkey_pem_file, hash, outfile):
    script_dir = get_script_dir()
    bl2_signing_tool = f'{get_python_exe()} {script_dir}/../tools/mcuboot_tools/imgtool.py'
    cmd = f'{bl2_signing_tool} sign-hash -k {privkey_pem_file} -d {hash} -o {outfile}'
    save_cmd('BL2 sign hash', cmd)
    run_cmd(cmd)

def bl2_sign(action_type, key_type, privkey_pem_file, pubkey_pem_file, signature, bin_file, partition_size, version, security_counter, sign_outfile, hash_outfile, pad=True):

    logging.debug(f'bl2 sign, action_type={action_type}, key_type={key_type}')
    logging.debug(f'privkey_pem_file={privkey_pem_file}, pubkey_pem_file={pubkey_pem_file}, signature={signature} security_counter={security_counter} version={version}')
    logging.debug(f'bin={bin_file}, sign_outfile={sign_outfile} hash_outfile={hash_outfile} partition_size={partition_size}')

    if action_type == 'hash':
        key_opt = f''
        hash_outfile_opt = f'--hash_outfile {hash_outfile}'
    elif action_type == 'sign_from_sig':
        # The signature is supplied through --signature, so imgtool must not try
        # to load a private key: the pack-server side of the split-signing flow
        # does not have one. imgtool then takes the embedded public key from
        # --pubkeyfile (image.py: key is None -> pubkeyfile).
        key_opt = f''
        hash_outfile_opt = ''
    else:
        key_opt = f'-k {privkey_pem_file}'
        hash_outfile_opt = ''

    if action_type == 'sign_from_sig':
        signature_opt = f'--signature {signature}'
    else:
        signature_opt = ''

    # --pad fills the output up to --slot-size and writes the mcuboot swap
    # trailer at the slot end. The executable image (primary_all / XIP ota) needs
    # it, but the compressed-overwrite ota image does not: BL2 sizes everything
    # from the header (ih_hdr_size/ih_img_size) and arms the install via the
    # ota_control confirm record, never the ota-slot trailer (see loader.c). So
    # pass pad=False for the compressed ota image to avoid padding a ~1.3MB blob
    # up to the full 3MB partition. --slot-size is kept either way for the
    # "image too big for slot" safety check.
    pad_opt = f'--pad' if pad else f''

    script_dir = get_script_dir()
    bl2_signing_tool_dir = f'{get_python_exe()} {script_dir}/../tools/mcuboot_tools/imgtool.py'
    cmd = f'{bl2_signing_tool_dir} sign {key_opt} --public-key-format full --max-align 8 --align 1 --version {version} --security-counter {security_counter} --pad-header --header-size 0x1000 --slot-size {partition_size} {pad_opt} --boot-record SPE --endian little --encrypt-keylen 128 {bin_file} {sign_outfile} --action_type {action_type} {signature_opt} --pubkeyfile {pubkey_pem_file} {hash_outfile_opt}'
    save_cmd(f'BL2 sign ({action_type}) {bin_file} -> {sign_outfile}', cmd)
    run_cmd(cmd)
    return
