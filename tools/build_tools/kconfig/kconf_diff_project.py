#!/usr/bin/env python3
#
# Generate a minimal project config file that contains only differences
# between a board-level defconfig and a full project config.
#
# This is intended to be used to maintain a small
#   projects/<proj>/ap/config/<soc>/defconfig
# which overrides
#   ap/middleware/soc/<soc>/<soc>.defconfig
#
# SPDX-FileCopyrightText: 2025 BEKEN
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import sys
from typing import List

import kconfiglib.core as kconfiglib


def _load_kconfig(kconfig_path: str, config_files: List[str], srctree: str | None) -> kconfiglib.Kconfig:
    """
    Create a Kconfig instance for the given top-level Kconfig file and load
    the provided config files (defconfig, project config, etc.) in order.
    """

    # Ensure Kconfiglib can resolve relative 'source' paths correctly
    if srctree:
        os.environ["srctree"] = srctree

    kconf = kconfiglib.Kconfig(kconfig_path, suppress_traceback=True)

    for cfg in config_files:
        if cfg and os.path.exists(cfg):
            # merge configurations in the given order
            kconf.load_config(cfg, replace=False)

    return kconf


def _bool_default_str(sym: kconfiglib.Symbol) -> str | None:
    """
        calculate the default value of a BOOL/TRISTATE symbol when no defconfig/config is loaded
    """

    if sym.orig_type not in (kconfiglib.BOOL, kconfiglib.TRISTATE):
        return None

    for default, _cond in sym.defaults:
        try:
            if getattr(default, "str_value", None) == "y":
                return "y"
        except Exception:
            continue

    return "n"


def _write_project_config(
    kconf_base: kconfiglib.Kconfig,
    kconf_full: kconfiglib.Kconfig,
    out_path: str,
) -> None:
    """
    Compare 'base' (board defconfig) and 'full' (defconfig + full project config)
    configurations, and write only the differing symbols to out_path in .config
    format.
    """

    lines: list[str] = []
    lines.append("#\n")
    lines.append("# Automatically generated project default config. You can update it with menuconfig [D] option.\n")
    lines.append("# This file contains only differences compared to board defconfig.\n")
    lines.append("#\n")

    # Iterate over symbols defined in the 'full' configuration. For each symbol,
    # compare the final value with the value from the base configuration.
    for sym_full in kconf_full.unique_defined_syms:
        # Skip symbols without a name or with env source
        if not sym_full.name or sym_full.env_var is not None:
            continue

        sym_base = kconf_base.syms.get(sym_full.name)
        if sym_base is None:
            # If the symbol does not exist in base tree, treat it as having the
            # default "unset" value and no explicit user assignment there.
            base_val = ""
            base_user = None
        else:
            base_val = sym_base.str_value
            # user_value is the explicit user assignment
            base_user = getattr(sym_base, "user_value", None)

        full_val = sym_full.str_value

        # get the default value of the symbol, only for BOOL/TRISTATE
        default_val = _bool_default_str(sym_full)

        # rule 1：when base_val == full_val and board is not explicitly configured (base_user is None),
        #   if full_val is different from the Kconfig default value, it is considered that the project has made a "non-default configuration", and needs to be written into defconfig.
        #
        # typical scenarios:
        #   - Kconfig default y, board is calculated to n due to dependencies
        #   - the project explicitly sets it to n
        #   - base_val == full_val == "n", but default_val == "y"
        #   => full_val != default_val, and base_user is None => write into defconfig
        if base_val == full_val:
            if (
                default_val is not None
                and base_user is None
                and full_val != default_val
            ):
                # meet the rule 1
                pass
            else:
                continue

        # rule 2：when base_val != full_val and board is not explicitly configured (base_user is None),
        #   if full_val is exactly equal to the Kconfig default value, it is considered that the project has simply "returned to the default value", 
        #   and does not need to be written into defconfig.
        #
        # typical scenarios:
        #   - Kconfig default y
        #   - board is calculated to n due to dependencies (base_val="n")
        #   - the project opens related dependencies, and full_val is restored to default y
        #   => full_val == default_val, and base_user is None => do not write into defconfig
        if base_val != full_val:
            if (
                default_val is not None
                and base_user is None
                and full_val == default_val
            ):
                continue

        cfg_string = sym_full.config_string
        if not cfg_string:
            continue

        lines.append(cfg_string)

    # Ensure there is a trailing newline
    if lines and not lines[-1].endswith("\n"):
        lines[-1] += "\n"

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "w") as f:
        f.writelines(lines)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate minimal defconfig as diff between defconfig and full config"
    )
    parser.add_argument(
        "--kconfig",
        required=True,
        help="Path to top-level Kconfig file",
    )
    parser.add_argument(
        "--defconfig",
        required=True,
        help="Path to board-level defconfig file",
    )
    parser.add_argument(
        "--full-config",
        required=True,
        help="Path to full project config (after menuconfig)",
    )
    parser.add_argument(
        "--out",
        required=True,
        help="Path to output defconfig file",
    )
    parser.add_argument(
        "--srctree",
        help="Optional srctree path for Kconfiglib (usually ARMINO_PATH)",
        default=None,
    )

    args = parser.parse_args()

    # View A: only board defconfig
    kconf_base = _load_kconfig(
        kconfig_path=args.kconfig,
        config_files=[args.defconfig],
        srctree=args.srctree,
    )

    # View B: board defconfig + full project config
    kconf_full = _load_kconfig(
        kconfig_path=args.kconfig,
        config_files=[args.defconfig, args.full_config],
        srctree=args.srctree,
    )

    _write_project_config(kconf_base, kconf_full, args.out)


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"kconf_diff_project failed: {e}", file=sys.stderr)
        sys.exit(1)

