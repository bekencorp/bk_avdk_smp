set(incs)
set(srcs)
set(priv_incs)

list(APPEND incs
	include
)

if (CONFIG_BK_H264E)

	list(APPEND srcs
		src/h264_encoder_api.c
	)
	if (CONFIG_BK_H264E_DEMO)
		list(APPEND srcs
			bk_test_h264e.c
		)
	endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_INCLUDE_DIRS "${priv_incs}" PRIV_REQUIRES avdk_driver)

if (CONFIG_BK_H264E)
target_compile_options(${COMPONENT_LIB}
	PRIVATE
	-Os
	-Wno-int-to-pointer-cast
	-Wno-pointer-to-int-cast
	-Wno-unused-variable
)

target_compile_definitions(${COMPONENT_LIB}
	PRIVATE
	BK_H265E
	__FREERTOS__
	SUBDEV_CHAR=1
	FREERTOS=1
	DUMMY_BUILD=0
	ISP_PLATFORM=FPGA
	OFFLINE_TEST=0
	STATIC_BUILD=1
	BUILD_SHARED_LIBS=OFF
)
endif()