#!/usr/bin/env python3

import logging

from .ppc import *
from .gen_license import get_license
from .common import *

ppc_keys = [
    'Device',
    'Secure',
    'Privilege'
]

gpio_keys = [
    'GPIO',
    'Device'
]

def gen_ppc_const(ppc, f):
    for dict in ppc.csv.dic_list:
        if dict["Secure"] == "TRUE":
            line = f'#define GEN_SECURITY_DEV_%s_IS_SECURE    %s' %(dict["Device"], "1\n")
        else:
            line = f'#define GEN_SECURITY_DEV_%s_IS_SECURE    %s' %(dict["Device"], "0\n")
        f.write(line)
        if dict["Privilege"] == "TRUE":
            line = f'#define GEN_SECURITY_DEV_%s_IS_PRIVILEGE    %s' %(dict["Device"], "1\n")
        else:
            line = f'#define GEN_SECURITY_DEV_%s_IS_PRIVILEGE    %s' %(dict["Device"], "0\n")
        f.write(line)

    empty_line(f)
    line = f'#define DEV_IS_SECURE(dev)    GEN_SECURITY_DEV_##dev##_IS_SECURE\n'
    f.write(line)
    line = f'#define DEV_ID_IS_SECURE(dev,id)    GEN_SECURITY_DEV_##dev##id##_IS_SECURE\n'
    f.write(line)
    line = f'#define DEV_IS_PRIVILEGE(dev)    GEN_SECURITY_DEV_##dev##_IS_PRIVILEGE\n'
    f.write(line)
    line = f'#define DEV_ID_IS_PRIVILEGE(dev, id)    GEN_SECURITY_DEV_##dev##id##_IS_PRIVILEGE\n'
    f.write(line)

def _pphs_master_overlay(ap_ppc_csv='ap_ppc.csv', ap_sec_csv='ap_ppro_sec.csv'):
    """Return (word, mask, value) for the PPHS master security bank.

    The AP table and its PPHS Secure mapping are copied next to the CP table by
    bk_prebuild.py, so the secure image can carry the AP master policy itself:
    only devices whose name ends with '_M' participate. Privilege is ignored
    because ppro_ap.csv has no master entries.
    """
    if not (os.path.exists(ap_ppc_csv) and os.path.exists(ap_sec_csv)):
        return None

    with open(ap_sec_csv, newline="") as sec_file:
        sec_rows = list(csv.DictReader(sec_file))
    # PPHS attribute banks start at reg4; a PPRO table uses reg4 for GPIO.
    if not any(int(row['Reg'], 16) == 4 for row in sec_rows):
        return None

    bit_map = {}
    for row in sec_rows:
        # A device may own several bits (SDIO_M uses two), so keep them all.
        bit_map.setdefault(row['Device'], []).append(
            (int(row['Reg'], 16) - 4, int(row['Bit'])))

    with open(ap_ppc_csv, newline="") as ppc_file:
        rows = list(csv.DictReader(ppc_file))

    word = None
    mask = 0
    value = 0
    for row in rows:
        device = row['Device']
        if not device.endswith('_M'):
            continue
        if device not in bit_map:
            logging.warning(f'{device}: no PPHS bit, skipped by the master overlay')
            continue
        for dev_word, dev_bit in bit_map[device]:
            if word is None:
                word = dev_word
            elif word != dev_word:
                logging.warning(f'{device}: master overlay spans several PPHS words, skipped')
                return None
            mask |= 1 << dev_bit
            if row['Secure'] != "TRUE":
                value |= 1 << dev_bit

    if word is None:
        return None

    return word, mask, value

def gen_ppc_overlay_file(outfile='_ppc_overlay.h'):
    """Emit the compile-time PPHS overlay consumed by bk_tfm_ppc.c.

    Kept as plain integer macros: they are encoded as Thumb2 immediates, so the
    overlay costs no RAM, no literal-pool access and no extra flash read on the
    low-power AP boot path.
    """
    overlay = _pphs_master_overlay()
    word, mask, value = overlay if overlay else (0, 0, 0)

    f = open(outfile, 'w')
    logging.debug(f'Create {outfile}')
    f.write(get_license())
    f.write('/* AP PPHS master security bank overlay, generated from the AP\n'
            ' * ppc.csv. bk_tfm_ppc.c applies it on top of the flash config so\n'
            ' * an OTA of tfm_s alone can carry the AP master policy.\n'
            ' * PPC_AP_OVERLAY_MASK == 0 disables the overlay. */\n')
    f.write(f'#define PPC_AP_OVERLAY_WORD    {word}u\n')
    f.write(f'#define PPC_AP_OVERLAY_MASK    0x{mask:08X}u\n')
    f.write(f'#define PPC_AP_OVERLAY_VALUE   0x{value:08X}u\n')
    f.close()

def gen_ppc_init_file(ppc):
    ppc_init_file_name = f'tfm_hal_ppc.h'
    f = open(ppc_init_file_name, 'w')

    logging.debug(f'Create {ppc_init_file_name}')
    f.write(get_license())

    line = f'static inline void tfm_hal_ppc_init(void)\n'
    f.write(line)
    line = f'{{\n'
    f.write(line)

    for dict in ppc.csv.dic_list:
        if dict["Secure"] == "TRUE":
            line = f'    bk_prro_set_secure(PRRO_DEV_%s, PRRO_SECURE);\n' %(dict["Device"])
            f.write(line)

        if dict["Privilege"] == "FALSE":
            line = f'    bk_prro_set_privilege(PRRO_DEV_%s, PRRO_NON_PRIVILEGED);\n' %(dict["Device"])
            f.write(line)

    line = f'}}'
    f.write(line)

def gen_ppc_config_file(ppc_csv='ppc.csv', gpio_dev_csv='gpio_dev.csv', outfile='_ppc.h'):
    if not os.path.exists(ppc_csv):
        logging.debug(f'ppc.csv not exists, stip it')
        return

    ppc = PPC(ppc_csv, gpio_dev_csv)
    f = open(outfile, 'w+')

    logging.debug(f'Create {outfile}')
    f.write(get_license())

    ppc.check_gpio_security()
    ppc.gen_ppc_bin()
    gen_ppc_const(ppc, f)
    gen_ppc_init_file(ppc)
    gen_ppc_overlay_file()
