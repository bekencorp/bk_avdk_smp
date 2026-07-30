target_include_directories(platform_s PUBLIC .)
target_sources(platform_s
	PRIVATE
		target_cfg.c
		tfm_interrupts.c
		wdt.c
)
if (BL2)
	target_include_directories(platform_bl2 PUBLIC .)
	target_sources(platform_bl2
	PRIVATE
		wdt.c
)
endif()

if (BL2_DOWNLOAD)
	target_include_directories(download PUBLIC .)
endif()