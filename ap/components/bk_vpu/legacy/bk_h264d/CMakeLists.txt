set(incs include)
set(srcs)
set(priv_incs)

armino_build_get_property(armino_path ARMINO_PATH)

if (CONFIG_BK_H264D)

list(APPEND incs
	include
)

list(APPEND srcs
	src/h264_decoder_api.c
)
if (CONFIG_BK_H264D_DEMO)
list(APPEND srcs
	bk_test_h264d.c
)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_INCLUDE_DIRS "${priv_incs}" PRIV_REQUIRES avdk_driver)

if(CONFIG_BK_H264D)
target_compile_options(${COMPONENT_LIB}
	PRIVATE
	-Os
	-Wno-int-to-pointer-cast
	-Wno-pointer-to-int-cast
)

target_compile_definitions(${COMPONENT_LIB}
	PRIVATE
	BK_H264D
	__FREERTOS__
	BUILD_SHARED_LIBS=OFF
)
endif()