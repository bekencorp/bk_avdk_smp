armino_build_get_property(armino_soc ARMINO_SOC)

set(incs include src/core)
set(srcs)

list(APPEND srcs
	src/bk_gpu.c
	src/core/drv_vglite.c
	src/bk_gpu_ctlr_default.c
)

armino_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${incs}" 
	PRIV_REQUIRES avdk_driver media_service avdk_utils
)