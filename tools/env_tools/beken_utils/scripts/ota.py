#!/usr/bin/env python3
from .parse_csv import *

# Tail of the 4K BL2 manifest slot (written by bl1_sign.write_bl2_version_stamps).
# Keep in sync with bl2_version.c.
BL2_INTERNAL_VERSION_STAMP_OFFSET = 0xFE0  # internal SDK version, magic 'BL2I'
BL2_VERSION_STAMP_OFFSET = 0xFF0           # customer version, magic 'BL2V'

ota_keys = [
    'strategy',
    'encrypt',  
    'app_security_counter',
    'app_version',
    'bootloader_ota',
    'bootloader_ota_version',
]

class OTA(list):

    def __getitem__(self, key):
        return self.csv.dic[key]

    def get_app_security_counter(self):
        return int(self.csv.dic['app_security_counter'])

    def get_bl2_security_counter(self):
        return 4

    def get_strategy(self):
        return self.csv.dic['strategy']

    def get_encrypt(self):
        return (self.csv.dic['encrypt'].upper() == 'TRUE')

    def get_version(self):
        return self.csv.dic['app_version']

    def get_boot_ota(self):
        if 'bootloader_ota' in self.csv.dic :
            return (self.csv.dic['bootloader_ota'] == 'TRUE')
        else:
            return False

    def get_bootloader_ota_version(self):
        # Display/comparison only, never anti-rollback. None when absent/empty.
        return self.csv.dic.get('bootloader_ota_version', '').strip() or None

    def is_overwrite(self):
        if(self.csv.dic['strategy'].upper() == 'OVERWRITE'):
            return True
        else:
            return False

    def __init__(self, csv_file):
        self.csv = Csv(csv_file, False, ota_keys)
