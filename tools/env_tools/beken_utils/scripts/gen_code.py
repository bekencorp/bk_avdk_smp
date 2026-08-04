#!/usr/bin/env python3

from .gen_mpc import *
from .gen_ota import *
from .gen_otp_map import *
from .gen_partition import *
from .gen_ppc import *
from .gen_security import *
from .partition import *


def gen_code():
    gen_ppc_config_file("ppc.csv", "gpio_dev.csv", "_ppc.h")
    gen_mpc_config_file("mpc.csv", "_mpc.h")
    gen_security_config_file("security.csv", "security.h")
    gen_ota_config_file("ota.csv", "_ota.h")
    gen_otp_map_file()

    s = Security("security.csv")
    o = OTA("ota.csv")

    # Generate the partition PHY header and layout in one shot, matching the
    # BK7234N gen_code() flow so that a single `gen all` fully produces every
    # generated artifact (used directly during security deployment).
    # File names follow the BK7259 convention (partitions_gen.h is the PHY
    # #define header consumed by the TF-M stub / downloader; partitions_partition.h
    # is the layout table), so the output stays consumable by the rest of the build.
    p = Partitions(
        "partitions.csv",
        o.get_strategy(),
        o.get_boot_ota(),
        s.secureboot_en,
        s.crc_en,
    )
    gen_partitions_hdr_file(p, "partitions_gen.h")
    gen_partitions_layout_file(p, "partitions_partition.h")
