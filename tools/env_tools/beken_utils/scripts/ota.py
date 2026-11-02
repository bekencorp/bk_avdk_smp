#!/usr/bin/env python3
from .parse_csv import *

# Offset of the stamp inside the 4K BL2 manifest slot
# (see bl1_sign.write_bl2_version_stamp).
BL2_VERSION_STAMP_OFFSET = 0xFF0

ota_keys = [
    'strategy',
    'encrypt',  
    'app_security_counter',
    'app_version',
    'bootloader_ota',
    'bootloader_version',
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

    def get_bootloader_version(self):
        # Display/comparison only; never used for anti-rollback. None means the
        # key is absent or empty: the caller skips stamping instead of writing a
        # fake 0.0.0 the reader cannot tell apart from a real version.
        return self.csv.dic.get('bootloader_version', '').strip() or None

    def is_overwrite(self):
        if(self.csv.dic['strategy'].upper() == 'OVERWRITE'):
            return True
        else:
            return False

    def __init__(self, csv_file):
        self.csv = Csv(csv_file, False, ota_keys)
