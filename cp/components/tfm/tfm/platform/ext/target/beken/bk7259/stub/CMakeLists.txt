target_include_directories(platform_region_defs INTERFACE .)
target_include_directories(platform_s PUBLIC .)

if (BL2_DOWNLOAD)
    target_include_directories(download PUBLIC .)
endif()
