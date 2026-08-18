#!/usr/bin/env python3

import logging
from .ota import *
from .gen_license import get_license
from .common import *

# Magic written to the ota_control confirm slot by the device receiver and read
# by BL2 (bk_boot_read_ota_confirm). MUST equal OVERWRITE_CONFIRM in BL2.
OTA_CONFIRM = 0xA16D8FB0

def gen_ota_config_file(ota_csv, outfile):
    ota = OTA(ota_csv)
    f = open(outfile, 'w+')
    logging.debug(f'Create {outfile}')
    f.write(get_license())

    macro_name = f'CONFIG_OTA_OVERWRITE'
    if (ota.get_strategy().upper() == 'OVERWRITE'):
        line = f'#define %-45s %d\n' %(macro_name, 1)
        f.write(line)

        # Enable the BL2 confirm/anti-brick trigger (loader.c, bootutil_public.c)
        # and expose the confirm magic. Device receiver arms an update by writing
        # OVERWRITE_CONFIRM into ota_control; BL2 decompresses ota -> primary_all.
        macro_name = f'CONFIG_OTA_CONFIRM_UPDATE'
        line = f'#define %-45s %d\n' %(macro_name, 1)
        f.write(line)

        macro_name = f'OVERWRITE_CONFIRM'
        line = f'#define %-45s 0x%x\n' %(macro_name, OTA_CONFIRM)
        f.write(line)
    else:
        line = f'#define %-45s %d\n' %(macro_name, 0)
        f.write(line)

    macro_name = f'CONFIG_DIRECT_XIP'
    if (ota.get_strategy().upper() in ('XIP', 'XIP_FORCE_A')):
        line = f'#define %-45s %d\n' %(macro_name, 1)
        f.write(line)
        macro_name = f'XIP_SET'
        line = f'#define %-45s 0x%x\n' %(macro_name, 0xDEADBEEF)
        f.write(line)
        macro_name = f'XIP_MAGIC_TYPE'
        line = f'#define %-45s %d\n' %(macro_name, 1)
        f.write(line)
        macro_name = f'XIP_COPY_DONE_TYPE'
        line = f'#define %-45s %d\n' %(macro_name, 2)
        f.write(line)
        macro_name = f'XIP_IMAGE_OK_TYPE'
        line = f'#define %-45s %d\n' %(macro_name, 3)
        f.write(line)
        macro_name = f'XIP_ACTIVE'
        line = f'#define %-45s 0x%x\n' %(macro_name, 0xFFFF0000)
        f.write(line)
        macro_name = f'XIP_BACK'
        line = f'#define %-45s 0x%x\n' %(macro_name, 0x0)
        f.write(line)
        macro_name = f'XIP_UNACTIVE'
        line = f'#define %-45s 0x%x\n' %(macro_name, 0xFFFFFFFF)
        f.write(line)
    else:
        line = f'#define %-45s %d\n' %(macro_name, 0)
        f.write(line)

    macro_name = f'CONFIG_OTA_ENCRYPTED'
    if ota.get_encrypt():
        line = f'#define %-45s %d\n' %(macro_name, 1)
        f.write(line)
    else:
        line = f'#define %-45s %d\n' %(macro_name, 0)
        f.write(line)
