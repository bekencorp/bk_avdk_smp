#!/usr/bin/env python3

import os
import json
import logging
from .common import *
from .genbl1 import *
from .save_cmd import save_cmd

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
