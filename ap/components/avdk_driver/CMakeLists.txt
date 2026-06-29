set(incs include)
set(srcs "")

if (CONFIG_ISP)
	list(APPEND srcs
		src/isp/isp_core.c
		src/isp/isp_reg_dump.c
	)
	list(APPEND incs src/isp)
endif()

if (CONFIG_GPU)
	list(APPEND srcs
		src/gpu/gpu_core.c
	)
endif()

if (CONFIG_MIPI_CSI)
	list(APPEND srcs
		src/mipi_csi/mipi_csi_driver.c
	)
endif()

if (CONFIG_SOC_BK7259)
	list(APPEND incs
		include/soc/bk7259
	)
endif()

if (CONFIG_DPU_DRIVER)
	list(APPEND srcs
		src/dpu/dpu_core.c)

	if (CONFIG_DSI_DRIVER)
		list(APPEND srcs src/dpu/mipi_dsi_core.c)
		list(APPEND srcs src/dpu/mipi_dsi_hal.c)
	endif()
endif()

set(priv_req bk_common)

armino_component_register(INTERNAL_LIB SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}" unity avdk_utils bk_libs)


add_definitions(-Wno-unused-variable
                -Wno-unused-but-set-variable
                -Wno-int-to-pointer-cast
                -Wno-pointer-to-int-cast)
