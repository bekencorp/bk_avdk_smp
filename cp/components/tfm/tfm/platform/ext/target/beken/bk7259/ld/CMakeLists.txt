# Phase-1 uses Isolation Level 2 -> use the plain secure linker script.
# For L3, switch to tfm_isolation_l3.ld (kept in this dir for the evolution).
target_add_scatter_file(tfm_s
        $<$<AND:$<VERSION_LESS:${TFM_ISOLATION_LEVEL},3>,$<C_COMPILER_ID:GNU>>:bk7259_s.ld>
        $<$<AND:$<VERSION_EQUAL:${TFM_ISOLATION_LEVEL},3>,$<C_COMPILER_ID:GNU>>:tfm_isolation_l3.ld>
)
if (BL2)
        target_add_scatter_file(bl2 $<$<C_COMPILER_ID:GNU>:bk7259_bl2.ld>)
endif()

# Compressed-overwrite OTA (secureboot_overwrite) only. The scatter (.ld)
# preprocessor resolves #include "security.h"/"_ota.h" from platform_region_defs'
# interface include dirs, which point at the bk7259/stub copy (hardcoded XIP:
# CONFIG_DIRECT_XIP=1). Meanwhile the secure/BL2 C code sees the project-generated
# headers staged into PARTITION_HDR_DIR (DIRECT_XIP=0 for overwrite). That
# disagreement makes region_defs.h compute BL2_DS_RETENTION_SIZE differently for
# the .ld ASSERT vs the compiled .bl2_fastboot_retention section -> link error.
# Put PARTITION_HDR_DIR ahead of the stub for the scatter preprocess so the .ld
# uses the same config as the code. Only for OVERWRITE_ONLY (PARTITION_HDR_DIR
# holds no security.h for XIP, so this is a no-op there regardless, but the guard
# keeps the XIP link byte-identical). tfm_hal_ppc.h is not staged, so the secure
# PRRO stub is unaffected.
if (MCUBOOT_UPGRADE_STRATEGY STREQUAL "OVERWRITE_ONLY")
        target_include_directories(tfm_s_scatter BEFORE PRIVATE ${PARTITION_HDR_DIR})
        if (BL2)
                target_include_directories(bl2_scatter BEFORE PRIVATE ${PARTITION_HDR_DIR})
        endif()
endif()
