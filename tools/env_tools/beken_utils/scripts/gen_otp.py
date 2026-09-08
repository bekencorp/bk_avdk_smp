#!/usr/bin/env python3

import os
import json
import logging
from .common import *
from scripts.rotpk_hash import *

CONFIG_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'config')
FLASH_AES_KEY_CONFIG = os.path.join(CONFIG_DIR, 'flash_aes_key.txt')
OTP_CTRL_CONFIG = os.path.join(CONFIG_DIR, 'otp_ctrl_config.json')


def load_config_flash_aes_key():
    if os.path.isfile(FLASH_AES_KEY_CONFIG):
        with open(FLASH_AES_KEY_CONFIG) as f:
            key = ''.join(f.read().split())
        if key:
            return key
    return None


def load_otp_ctrl_content():
    if os.path.isfile(OTP_CTRL_CONFIG):
        with open(OTP_CTRL_CONFIG) as f:
            cfg = json.load(f)
        return cfg.get('content')
    return None


def apply_otp_ctrl_bits(security_ctrl, content):
    # content is the efuse value as a big-endian hex string, e.g. "20000049"
    # sets efuse bit0/3/6/29 (the high byte 0x20 -> bit29). Each efuse bit i
    # maps to the Security_Ctrl field with (byte = i // 8, bit = i % 8);
    # e.g. bit29 -> "3,5" (flash_aes_enable).
    value = int(content, 16)
    bit_to_field = {}
    for name, cfg in security_ctrl.items():
        byte_idx, bit_idx = cfg.split(',')[0:2]
        bit_to_field[int(byte_idx) * 8 + int(bit_idx)] = name

    for efuse_bit in range(value.bit_length()):
        if not (value >> efuse_bit) & 1:
            continue
        name = bit_to_field.get(efuse_bit)
        if name:
            b, i = security_ctrl[name].split(',')[0:2]
            security_ctrl[name] = f'{b},{i},1'
        else:
            logging.warning(
                f'otp_ctrl bit {efuse_bit} has no matching Security_Ctrl field')

def reverse_order(hex_str):
    hex_str_len = len(hex_str)
    word_len = hex_str_len // 8
    reverse_str = ''
    for i in range(word_len):
        idx = i << 3
        reverse_str = reverse_str + hex_str[idx + 6]
        reverse_str = reverse_str + hex_str[idx + 7]
        reverse_str = reverse_str + hex_str[idx + 4]
        reverse_str = reverse_str + hex_str[idx + 5]
        reverse_str = reverse_str + hex_str[idx + 2]
        reverse_str = reverse_str + hex_str[idx + 3]
        reverse_str = reverse_str + hex_str[idx + 0]
        reverse_str = reverse_str + hex_str[idx + 1]


    logging.debug(f'hex_str={hex_str}')
    logging.debug(f'reverse_str={reverse_str}')
    return reverse_str


def gen_otp_efuse_config_file(aes_type, flash_aes_key, pubkey_pem_file, secureboot_en, outfile):
    f = open(outfile, 'w+')
    logging.debug(f'Create {outfile}')

    otp_efuse_config = {
        "User_Operate_Enable":  "false",
        "Security_Ctrl_Enable": "true",
        "Security_Data_Enable": "true",

        "User_Operate":[],

        "Security_Ctrl":[{
            "secure_boot_enable":           "0,0,0",
            "secure_boot_debug_disable":    "0,1,0",
            "fast_boot_disable":            "0,2,0",
            "boot_mode":                    "0,3,0",
            "secure_boot_clock_select":     "0,4,0",
            "random_delay_enable":          "0,5,0",
            "direct_jump_enable":           "0,6,0",
            "boot_critical_error": "0,7,0",
            "attack_nmi_enable":            "2,4,0",
            "spi_to_ahb_disable":           "2,5,0",
            "auto_reset_enable[0]":         "2,6,0",
            "auto_reset_enable[1]":         "2,7,0",
            "flash_aes_enable":             "3,5,0",
            "spi_download_disable":         "3,6,0",
            "swd_disable":                  "3,7,0"
        }],

        "Security_Data":[]
    }

    data = {}

    if aes_type == 'FIXED':
        otp_efuse_config["Security_Ctrl"][0].update({"flash_aes_enable":"3,5,1"})
        data["name"] = "flash_aes_key"
        data["mode"] = "write"
        data["permission"] = "WR" #TODO change to NA
        data["start_addr"] = "0x4B100460"
        data["last_valid_addr"] = "0x4B100480"
        data["byte_len"] = "0x20"
        data["data"] = flash_aes_key
        data["data_type"] = "hex"
        data["status"] = "true"
        otp_efuse_config["Security_Data"].append(data)
    elif aes_type == 'RANDOM':
        otp_efuse_config["Security_Ctrl"][0].update({"flash_aes_enable":"3,5,1"})
    else:
        pass

    if secureboot_en:
        h = Rotpk_hash(pubkey_pem_file)
        hash_dict = h.gen_rotpk_hash()
        bl1_pk_hash = hash_dict['bl1_rotpk_hash']
        bl2_pk_hash = hash_dict['bl2_rotpk_hash']

        otp_ctrl_content = load_otp_ctrl_content()
        if otp_ctrl_content:
            apply_otp_ctrl_bits(otp_efuse_config["Security_Ctrl"][0], otp_ctrl_content)
        else:
            otp_efuse_config["Security_Ctrl"][0].update({"secure_boot_enable":"0,0,1"})
            otp_efuse_config["Security_Ctrl"][0].update({"boot_mode":"0,3,1"})
            otp_efuse_config["Security_Ctrl"][0].update({"direct_jump_enable":"0,6,1"})

        data = data.copy()
        data["name"] = "bl1_rotpk_hash"
        data["mode"] = "write"
        data["permission"] = "WR" #TODO change to RO
        data["start_addr"] = "0x4B100528"
        data["last_valid_addr"] = "0x4B100548"
        data["byte_len"] = "0x20"
        data["data"] = bl1_pk_hash
        data["data_type"] = "hex"
        data["status"] = "true"
        otp_efuse_config["Security_Data"].append(data)

        data = data.copy()
        data["name"] = "bl2_rotpk_hash"
        data["mode"] = "write"
        data["permission"] = "WR" #TODO change to RO
        data["start_addr"] = "0x4B100548"
        data["last_valid_addr"] = "0x4B100568"
        data["byte_len"] = "0x20"
        data["data"] = bl2_pk_hash
        data["data_type"] = "hex"
        data["status"] = "true"
        otp_efuse_config["Security_Data"].append(data)
    else:
        h = Rotpk_hash(pubkey_pem_file)
        hash_dict = h.gen_rotpk_hash()
        bl2_pk_hash = hash_dict['bl2_rotpk_hash']
        data = data.copy()
        data["name"] = "bl2_rotpk_hash"
        data["mode"] = "write"
        data["permission"] = "WR" #TODO change to RO
        data["start_addr"] = "0x4B100548"
        data["last_valid_addr"] = "0x4B100568"
        data["byte_len"] = "0x20"
        data["data"] = bl2_pk_hash
        data["data_type"] = "hex"
        data["status"] = "true"
        otp_efuse_config["Security_Data"].append(data)

    json_str = json.dumps(otp_efuse_config, indent=4)
    with open('otp_efuse_config.json', 'w',newline="\n") as file:
        file.write(json_str)
        if (secureboot_en):
            if (flash_aes_key):
                aes = f"\n# flash aes key in little endian:{reverse_order(flash_aes_key)}\n"
                file.write(aes)

            bl1 = f"# bl1_rotpk_hash in little endian:{reverse_order(bl1_pk_hash)}\n"
            file.write(bl1)
            bl2 = f"# bl2_rotpk_hash in little endian:{reverse_order(bl2_pk_hash)}\n"
            file.write(bl2)
    file.close()
