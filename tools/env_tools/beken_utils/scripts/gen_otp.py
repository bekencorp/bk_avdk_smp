#!/usr/bin/env python3

import logging
from .common import *
from scripts.rotpk_hash import *

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


def add_security_data(otp_efuse_config, name, start_addr, byte_len, data, permission="WR", mode="write"):
    entry = {
        "name": name,
        "mode": mode,
        "permission": permission,
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

    otp_efuse_config = {
        "User_Operate_Enable":  "false",
        "Security_Data_Enable": "true",

        "User_Operate":[],
        "Security_Data":[]
    }

    data = {}
    if secureboot_en:
        if  aes_type == 'FIXED':
            efuse_data = "28000000"   #if want to configure other value, please configure this item_value(0x28000008)
        else:
            efuse_data = "08000000"   #TODO temp use 8000000, the final value to be used 8000008 (bit3 reps: open secureboot)

        add_security_data(otp_efuse_config, "efuse", 0x44850014, 0x4, efuse_data)

    if aes_type == 'FIXED':
        flash_aes_key = get_effective_flash_aes_key(flash_aes_key)
        if flash_aes_key != None:
            if len(flash_aes_key) == 128:
                flash_aes_key1 = flash_aes_key[:64]
                flash_aes_key2 = flash_aes_key[64:]
                add_security_data(otp_efuse_config, "flash_aes_key2", 0x42100560, 0x20, flash_aes_key2)
                add_security_data(otp_efuse_config, "flash_aes_key1", 0x42100580, 0x20, flash_aes_key1)
            else:
                flash_aes_key_byte_len = get_flash_aes_key_byte_len(flash_aes_key)
                add_security_data(otp_efuse_config, "flash_aes_key", 0x42100580, flash_aes_key_byte_len, flash_aes_key)

    if secureboot_en:
        h = Rotpk_hash(pubkey_pem_file)
        hash_dict = h.gen_rotpk_hash()
        bl1_pk_hash = (hash_dict['bl1_rotpk_hash'])
        bl2_pk_hash = (hash_dict['bl2_rotpk_hash'])

        add_security_data(otp_efuse_config, "bl1_rotpk_hash", 0x42100628, 0x20, bl1_pk_hash)
        add_security_data(otp_efuse_config, "bl2_rotpk_hash", 0x42100648, 0x20, bl2_pk_hash)
    else:
        h = Rotpk_hash(pubkey_pem_file, False)
        hash_dict = h.gen_rotpk_hash()
        bl2_pk_hash = (hash_dict['bl2_rotpk_hash'])
        add_security_data(otp_efuse_config, "bl2_rotpk_hash", 0x42100648, 0x20, bl2_pk_hash)

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
