target_include_directories(platform_region_defs INTERFACE .)
target_include_directories(platform_s PUBLIC .)

# BK7259 secure-world drivers are now provided self-contained by armino_min/
# (see ../armino_min/). The previous gap-fill shims (anti_tamper_stub.c +
# sys_drv_int_shim.c) have been merged into armino_min/hal/stub_sca.c,
# sys_min.c and ckmn_min.c, so they are no longer compiled here (avoids
# duplicate-symbol).
if (BL2)
    # BL2 flash_map.c / bl2_main.c include hal_flash.h / hal_efuse.h which live
    # under common/download/inc/hal. These headers are independent of the
    # BL2_DOWNLOAD feature (which is OFF for bk7259), so add the include path.
    target_include_directories(platform_bl2 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/.
        ${CMAKE_CURRENT_LIST_DIR}/../../common/download/inc/hal
        ${CMAKE_CURRENT_LIST_DIR}/../../common/download/inc)
endif()

if (BL2_DOWNLOAD)
    target_include_directories(download PUBLIC .)
endif()
