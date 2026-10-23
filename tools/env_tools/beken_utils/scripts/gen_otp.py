#!/usr/bin/env python3

import copy
import logging
from pathlib import Path

import yaml

from .common import *
from scripts.rotpk_hash import *

SOC_TYPE = 'bk7259'

_OTP_CONFIG_PATH = Path(__file__).with_name('gen_otp_config.yaml')
_OTP_CONFIG = None


def load_otp_config():
    global _OTP_CONFIG
    if _OTP_CONFIG is None:
        with _OTP_CONFIG_PATH.open(encoding='utf-8') as f:
            _OTP_CONFIG = yaml.safe_load(f) or {}
    return _OTP_CONFIG


def get_soc_config():
    config = load_otp_config()
    if SOC_TYPE not in config:
        logging.error(f'{SOC_TYPE} not found in {_OTP_CONFIG_PATH}')
        exit(1)
    return config[SOC_TYPE]


def get_security_ctrl():
    """Return a fresh copy of the Security_Ctrl bit table, so updating one
    field does not pollute the cached config."""
    return copy.deepcopy(get_soc_config()['security_ctrl'])


def update_security_ctrl_last_value(security_ctrl, field_name, value):
    """Update only the last segment of a comma-separated Security_Ctrl value."""
    parts = security_ctrl[field_name].split(',')
    parts[-1] = str(value)
    security_ctrl[field_name] = ','.join(parts)


def get_otp_addr(data_name):
    security_data = get_soc_config()['security_data']
    if data_name not in security_data:
        supported = ', '.join(sorted(security_data.keys()))
        logging.error(f'unsupported Security_Data "{data_name}", supported: {supported}')
        exit(1)
    return security_data[data_name]


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


def get_flash_aes_key_byte_len(flash_aes_key):
    if flash_aes_key == None:
        return 0
    return len(flash_aes_key) // 2


def get_effective_flash_aes_key(flash_aes_key):
    if flash_aes_key == None:
        return None

    return flash_aes_key


def add_security_data(otp_efuse_config, name, data, byte_len=None):
    """Append one Security_Data record; the address comes from gen_otp_config.yaml."""
    otp_addr = get_otp_addr(name)
    start_addr = int(otp_addr['start_addr'], 16)
    if byte_len is None:
        byte_len = int(otp_addr['byte_len'], 16)

    entry = {
        "name": name,
        "mode": "write",
        "permission": "RO",
        "start_addr": hex(start_addr),
        "last_valid_addr": hex(start_addr + byte_len),
        "byte_len": hex(byte_len),
        "data": data,
        "data_type": "hex",
        "status": "true"
    }
    otp_efuse_config["Security_Data"].append(entry)


def gen_otp_efuse_config_file(aes_type, flash_aes_key, pubkey_pem_file, secureboot_en, boot_ota, outfile):
    f = open(outfile, 'w+')
    logging.debug(f'Create {outfile}')

    security_ctrl = get_security_ctrl()

    otp_efuse_config = {
        "User_Operate_Enable":  "false",
        "Security_Ctrl_Enable": "true",
        "Security_Data_Enable": "true",
        "User_Operate":[],

        "Security_Ctrl":[security_ctrl],

        "Security_Data":[]
    }

    # if secureboot_en:
    #     if  aes_type == 'FIXED':
    #         efuse_data = "28000000"   #if want to configure other value, please configure this item_value(0x28000008)
    #     else:
    #         efuse_data = "08000000"   #TODO temp use 8000000, the final value to be used 8000008 (bit3 reps: open secureboot)

    #     add_security_data(otp_efuse_config, "efuse", 0x44850014, 0x4, efuse_data)

    if aes_type == 'FIXED':
        update_security_ctrl_last_value(security_ctrl, "flash_aes_enable", 1)
        flash_aes_key = get_effective_flash_aes_key(flash_aes_key)
        if flash_aes_key != None:
            if len(flash_aes_key) == 128:
                flash_aes_key1 = flash_aes_key[:64]
                flash_aes_key2 = flash_aes_key[64:]
                add_security_data(otp_efuse_config, "flash_aes_key2", flash_aes_key2)
                add_security_data(otp_efuse_config, "flash_aes_key1", flash_aes_key1)
            else:
                update_security_ctrl_last_value(security_ctrl, "flash_aes_mode", 1)
                flash_aes_key_byte_len = get_flash_aes_key_byte_len(flash_aes_key)
                add_security_data(otp_efuse_config, "flash_aes_key", flash_aes_key, flash_aes_key_byte_len)

    if secureboot_en:
        h = Rotpk_hash(pubkey_pem_file)
        hash_dict = h.gen_rotpk_hash()
        bl1_pk_hash = (hash_dict['bl1_rotpk_hash'])
        bl2_pk_hash = (hash_dict['bl2_rotpk_hash'])

        add_security_data(otp_efuse_config, "bl1_rotpk_hash", bl1_pk_hash)
        add_security_data(otp_efuse_config, "bl2_rotpk_hash", bl2_pk_hash)
    else:
        h = Rotpk_hash(pubkey_pem_file, False)
        hash_dict = h.gen_rotpk_hash()
        bl2_pk_hash = (hash_dict['bl2_rotpk_hash'])
        add_security_data(otp_efuse_config, "bl2_rotpk_hash", bl2_pk_hash)

    json_str = json.dumps(otp_efuse_config, indent=4)
    with open('otp_efuse_config.json', 'w',newline="\n") as file:
        file.write(json_str)
        if (secureboot_en):
            if (flash_aes_key):
                aes = f"\n# flash_aes_key in little endian:{reverse_order(flash_aes_key)}\n"
                file.write(aes)

            bl1 = f"# bl1_rotpk_hash in little endian:{reverse_order(bl1_pk_hash)}\n"
            file.write(bl1)
            bl2 = f"# bl2_rotpk_hash in little endian:{reverse_order(bl2_pk_hash)}\n"
            file.write(bl2)
            file.write(f"# data[\"mode\"] = \"write\" if want to read, please configure to \"read\"\n")
    file.close()
