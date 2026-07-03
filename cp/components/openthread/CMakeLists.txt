if (CONFIG_OPENTHREAD)

    armino_build_get_property(project_dir PROJECT_DIR)

    set(incs
	openthread/include
	openthread/examples/platforms
	openthread/examples/platforms/utils
	openthread/include/openthread
	openthread/include/openthread/platform
	openthread/src
	openthread/src/core/backbone_router
	openthread/src/core
	openthread/src/include
	openthread/src/lib
	config
	include
	private_include
	openthread/third_party/tcplp/bsdtcp
	openthread/third_party/tcplp/lib
	openthread/openthread/include
	${project_dir}/main
    )

    set(src_dirs
	openthread/examples/platforms/utils
	openthread/src/core/api
	openthread/src/core/backbone_router
	openthread/src/core/common
	openthread/src/core/crypto
	openthread/src/core/diags
	openthread/src/core/instance
	openthread/src/core/mac
	openthread/src/core/radio
	openthread/src/core/thread
	openthread/src/core/utils
	openthread/src/lib/hdlc
	openthread/src/lib/spinel
	openthread/src/core/coap
	openthread/src/core/meshcop
	openthread/src/core/net
	openthread/src/lib/platform
	openthread/examples/apps/cli
	openthread/src/cli
	openthread/third_party/tcplp/bsdtcp
	openthread/third_party/tcplp/lib
	openthread/third_party/tcplp/bsdtcp/cc
	src/port
	src
    )

    set(exclude_srcs
	openthread/examples/platforms/utils/logging_rtt.c
	openthread/examples/platforms/utils/soft_source_match_table.c
	openthread/src/core/instance/extension_example.cpp
	openthread/src/core/common/extension_example.cpp
	openthread/examples/apps/cli/main.c
	openthread/examples/apps/cli/cli_uart.cpp
    )

    if(CONFIG_OPENTHREAD_UNIT_TEST)
	list(APPEND src_dirs
	    src/port/test/src
	)

	list(APPEND incs
	    src/port/test/inc
	)
    endif()

endif()

#string(TIMESTAMP OT_BUILD_TIMESTAMP " %Y-%m-%d %H:%M:%S UTC" UTC)

armino_component_register(SRC_DIRS "${src_dirs}"
    EXCLUDE_SRCS "${exclude_srcs}"
    INCLUDE_DIRS "${incs}"
    PRIV_REQUIRES bk_common bk_mac802154 psa_mbedtls driver bk_cli bk_rtos os_source cmsis bk_phy temp_detect
)

if(CONFIG_OPENTHREAD)
    #Here used a common configuration for both FTD and MTD
    #Maybe we should create sperate configuration file for each if necessary in the future.
    set(config_file "openthread-core-bk7239n-config.h")
    if(CONFIG_OPENTHREAD_FTD)
	set(device_type "OPENTHREAD_FTD=1")
    elseif(CONFIG_OPENTHREAD_MTD)
	set(device_type "OPENTHREAD_MTD=1")
    endif()

    target_compile_definitions(
	${COMPONENT_LIB}
	PUBLIC
	"OPENTHREAD_CONFIG_FILE=\"${config_file}\""
	"OPENTHREAD_PROJECT_LIB_CONFIG_FILE=\"${config_file}\""
	"${device_type}"
	PRIVATE
	"PACKAGE_NAME=\"OPENTHREAD-BK7239N\""
	"PACKAGE_VERSION=\"V0.1\""
	"OPENTHREAD_BUILD_DATETIME=\"${OT_BUILD_TIMESTAMP}\""
	"OT_FULL_LOGS=ON"
    )
endif()

#unset(c_compile_options)
list(APPEND c_compile_options "")
armino_build_set_property(C_COMPILE_OPTIONS "${c_compile_options}" APPEND)


