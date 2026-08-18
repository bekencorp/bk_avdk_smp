#!/usr/bin/env python3
import os
import logging
import csv
import string

from .common import *
from .parse_csv import *

security_keys = [
    'secureboot_en',
    'sig_verify_en',
    'flash_aes_type',
    'flash_aes_mode',
    'crc_en',
    'flash_aes_key',
    'root_key_type',
    'root_pubkey',
    'root_privkey',
]

class Security(dict):

    def __getitem__(self, key):
        return self.csv.dic[key];

    def __init__(self, csv_file):
        self.csv = Csv(csv_file, False, security_keys)
        self.parse_csv()

    def is_flash_aes_fixed(self):
        return (self.flash_aes_type.upper() == 'FIXED')

    def is_flash_aes_random(self):
        return (self.flash_aes_type.upper() == 'RANDOM')

    def is_valid_hex_string(self, value):
        return all(ch in string.hexdigits for ch in value)

    def get_effective_flash_aes_key(self, flash_aes_key):
        if flash_aes_key is None:
            return None

        if self.flash_aes_mode == '128' and len(flash_aes_key) == 128:
            logging.warning('flash_aes_mode=128, only the last 64 hex chars of flash_aes_key are used')
            return flash_aes_key[64:]

        return flash_aes_key

    def parse_csv(self):
        self.secureboot_en = parse_bool(self.csv.dic['secureboot_en'])

        # sig_verify_en controls the BootROM/BL2 magic written at flash 0x100:
        #   TRUE  -> "BK.SB" (require signature; BootROM + BL2)
        #   FALSE -> "BEKEN" (skip signature; BL2 still verifies image hash)
        # It is independent of secureboot_en (which decides whether the
        # secureboot partition layout / signing is built at all). Optional and
        # defaults to FALSE so existing security.csv files (and the R&D/customer
        # default of a non-verifying, plaintext image) keep working unchanged.
        self.sig_verify_en = parse_bool(self.csv.dic.get('sig_verify_en', 'FALSE'))

        self.flash_aes_type = self.csv.dic['flash_aes_type'].upper()
        self.flash_aes_mode = self.csv.dic.get('flash_aes_mode', 'AUTO').upper()

        if self.flash_aes_mode not in ('AUTO', '128', '256'):
            logging.error('Invalid flash_aes_mode: only support AUTO, 128 or 256')
            exit(1)

        self.raw_flash_aes_key = self.csv.dic['flash_aes_key']
        self.flash_aes_key = self.raw_flash_aes_key
        if self.flash_aes_type == 'FIXED':
            flash_aes_key_len = len(self.flash_aes_key)
            if flash_aes_key_len not in (64, 128):
                logging.error('Invalid AES key: key length should be 64 or 128')
                exit(1)

            if not self.is_valid_hex_string(self.flash_aes_key):
                logging.error('Invalid AES key: key should be a hex string')
                exit(1)

            if self.flash_aes_mode == '256' and flash_aes_key_len != 128:
                logging.error('Invalid AES key: flash_aes_mode=256 requires 128 hex chars')
                exit(1)

            self.flash_aes_key = self.get_effective_flash_aes_key(self.flash_aes_key)
        else:
            self.flash_aes_key = None

        # crc_en is optional: projects that don't declare it (e.g. the plain
        # `app` project and the board-default csv/security.csv) default to CRC
        # disabled, which matches the BK7259 hardware (flash CRC off) and the
        # legacy behaviour. Consumers compare against 'TRUE', so 'FALSE' is the
        # safe default. Mirrors the flash_aes_mode .get() default above and
        # avoids a KeyError that would break every non-secure build.
        self.crc_en = self.csv.dic.get('crc_en', 'FALSE').upper()
        self.bl1_root_key_type = self.csv.dic['root_key_type']
        self.bl1_root_pubkey = self.csv.dic['root_pubkey']
        self.bl1_root_privkey = self.csv.dic['root_privkey']

        self.bl2_root_key_type = self.csv.dic['root_key_type']
        self.bl2_root_pubkey = self.csv.dic['root_pubkey']
        self.bl2_root_privkey = self.csv.dic['root_privkey']

        if self.secureboot_en == True:
            if (self.bl1_root_key_type != '') and (self.bl1_root_key_type != 'ec256') and (self.bl1_root_key_type != 'rsa2048'):
                logging.error(f'Unknown root key type {self.root_key_type}, only support ec256, rsa2048')
                exit(1)
