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

if (CONFIG_H264D)
	list(APPEND srcs
		src/h264d/h264d_core.c
	)
endif()

if (CONFIG_JPEGE)
	list(APPEND srcs
		src/jpege/jpege_driver.c
		src/jpege/jpege_core.c
	)
endif()

if (CONFIG_JPEGD)
	list(APPEND srcs
		src/jpegd/jpegd_core.c
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

	# 注意：这里传回父目录的路径，必须以 driver 组件目录为基准
	# 所以需要带上前缀 dpu/，参考 general_dma 的写法
	list(APPEND incs
		src/dpu/include
		src/dpu/soc)

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
