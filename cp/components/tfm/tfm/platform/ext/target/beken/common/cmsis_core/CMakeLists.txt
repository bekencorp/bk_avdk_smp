target_compile_options(platform_s PUBLIC ${COMPILER_CMSE_FLAG})
target_include_directories(platform_s PUBLIC .)
target_sources(platform_s
    PRIVATE
        system_cmsdk_bk7236.c
        cache.c
        mpu.c
)
if (BL2)
    target_include_directories(platform_bl2 PUBLIC .)
    target_sources(platform_bl2
        PRIVATE
            system_cmsdk_bk7236.c
            cache.c
            mpu_bl2.c
    )
endif()

if (BL2_DOWNLOAD)
	target_include_directories(download PUBLIC .)
endif()