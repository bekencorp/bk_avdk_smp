target_include_directories(platform_s PUBLIC .)
target_sources(platform_s PRIVATE mem_port.c os_port.c printf.c printf_base.c str_arch.c)
target_compile_definitions(platform_s
    INTERFACE
        -DCONFIG_CUSTOMIZED_PRINTF
)

if (BL2)
    target_include_directories(platform_bl2 PUBLIC .)
    target_sources(platform_bl2 PRIVATE mem_port.c os_port.c printf.c printf_base.c str_arch.c)
endif()

if (BL2_DOWNLOAD)
    target_include_directories(download PUBLIC .)
    target_sources(download PRIVATE os_port.c)
endif()