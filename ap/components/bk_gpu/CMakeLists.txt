set(incs include)
set(srcs)

if (CONFIG_VG_LITE_GPU)
list(APPEND srcs
	src/bk_gpu.c
	src/bk_gpu_ctlr_default.c
)
endif()

armino_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${incs}" 
	PRIV_REQUIRES avdk_driver media_service avdk_utils avdk_driver
)