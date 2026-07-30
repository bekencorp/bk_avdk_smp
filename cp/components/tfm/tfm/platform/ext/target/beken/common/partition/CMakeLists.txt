target_include_directories(platform_s PUBLIC .)
target_sources(platform_s PRIVATE tfm_flash_partition.c)

if (BL2)
    target_include_directories(platform_bl2 PUBLIC .)
    target_sources(platform_bl2 PRIVATE tfm_flash_partition.c)
endif()

if (BL2_DOWNLOAD)
	target_include_directories(download PUBLIC .)
    target_sources(platform_bl2 PRIVATE tfm_flash_partition.c)
endif()