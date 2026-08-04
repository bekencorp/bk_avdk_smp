#!/usr/bin/env python3

import logging
from .security import Security
from .gen_license import get_license
from .common import empty_line

CRC_PHY2CODE_START = "\
#define FLASH_CEIL_ALIGN(v, align) ((((v) + ((align) - 1)) / (align)) * (align))\n\
#define FLASH_PHY2VIRTUAL_CODE_START(phy_addr) FLASH_CEIL_ALIGN(FLASH_PHY2VIRTUAL(FLASH_CEIL_ALIGN((phy_addr), 34)), CPU_VECTOR_ALIGN_SZ)\n\
"

# Identity mapping for SoCs with flash CRC disabled in hardware (e.g. BK7259).
NO_CRC_PHY2CODE_START = "\
#define FLASH_CEIL_ALIGN(v, align) ((((v) + ((align) - 1)) / (align)) * (align))\n\
#define FLASH_PHY2VIRTUAL_CODE_START(phy_addr) FLASH_CEIL_ALIGN((phy_addr), CPU_VECTOR_ALIGN_SZ)\n\
"

def define(name, value):
    return f'#define {name:<45} {value}\n'


def gen_security_config_file(security_csv, outfile):
    security = Security(security_csv)
    logging.debug(f'Create {outfile}')
    crc_en = security.crc_en == 'TRUE'

    with open(outfile, 'w') as f:
        f.write(get_license())
        f.write('#include "_ota.h"\n')
        f.write('#include "_ppc.h"\n')
        f.write('#include "_mpc.h"\n')
        f.write(f'#undef {"MCUBOOT_SIGN_RSA":<45}\n')

        if security.secureboot_en:
            f.write(define('CONFIG_SECUREBOOT', 1))

        if security.bl2_root_key_type == 'rsa2048':
            f.write(define('MCUBOOT_SIGN_RSA', 1))
            f.write(define('MCUBOOT_SIGN_RSA_LEN', 2048))
        elif security.bl2_root_key_type == 'rsa3072':
            f.write(define('MCUBOOT_SIGN_RSA', 1))
            f.write(define('MCUBOOT_SIGN_RSA_LEN', 3072))
        elif security.bl2_root_key_type == 'ec256':
            f.write(define('MCUBOOT_SIGN_EC256', 1))
        else:
            raise ValueError(f'unsupported bl2 key type: {security.bl2_root_key_type}')

        if security.is_flash_aes_fixed():
            code_encrypted = 1
        elif security.is_flash_aes_random():
            code_encrypted = 2
        else:
            code_encrypted = 0
        f.write(define('CONFIG_CODE_ENCRYPTED', code_encrypted))

        f.write(define('CONFIG_CPU_CRC_EN', int(crc_en)))
        if crc_en:
            f.write(define('FLASH_VIRTUAL2PHY(virtual_addr)', '((((virtual_addr) >> 5) * 34) + ((virtual_addr) & 31))'))
            f.write(define('FLASH_PHY2VIRTUAL(phy_addr)', '((((phy_addr) / 34) << 5) + ((phy_addr) % 34))'))
            f.write(define('CEIL_ALIGN_34(addr)', '(((addr) + 34 - 1) / 34 * 34)'))
            phy2code_start = CRC_PHY2CODE_START
        else:
            f.write(define('FLASH_VIRTUAL2PHY(virtual_addr)', '(virtual_addr)'))
            f.write(define('FLASH_PHY2VIRTUAL(phy_addr)', '(phy_addr)'))
            f.write(define('CEIL_ALIGN_34(addr)', '(addr)'))
            phy2code_start = NO_CRC_PHY2CODE_START

        empty_line(f)
        f.write(phy2code_start)
        empty_line(f)
