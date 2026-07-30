target_include_directories(platform_s PUBLIC .)

set(src
	hal_jtag.c
)

target_sources(platform_s PRIVATE ${src})

if (BL2)
	target_include_directories(platform_bl2 PUBLIC .)
	target_sources(platform_bl2 PRIVATE ${src})
endif()
