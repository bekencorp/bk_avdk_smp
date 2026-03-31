set(incs include)
set(srcs )

if (CONFIG_BK_CAMERA)
	list(APPEND srcs
		src/bk_isp_camera.c
		src/csi/isp_cam_sensor.c
		src/isp/bk_camera_isp_ctlr_default.c
		src/sensor/bk_camera_sensor.c
		src/sensor/bk_camera_bus.c
		src/utils/isp_camera_utils.c
	)
endif()

set(priv_req bk_common)

armino_component_register(INTERNAL_LIB SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}" unity include avdk_driver bk_peripheral avdk_utils)
