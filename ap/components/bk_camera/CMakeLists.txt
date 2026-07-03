set(incs include)
set(srcs )
set(req "")
set(priv_req bk_common unity include avdk_driver bk_peripheral avdk_utils)

if (CONFIG_BK_CAMERA)
	list(APPEND srcs
		src/bk_isp_camera.c
		src/isp/bk_camera_isp_ctlr_default.c
		src/sensor/bk_camera_sensor.c
		src/sensor/bk_camera_bus.c
		src/utils/isp_camera_utils.c
	)
endif()

if (CONFIG_ISP_FRAME_SESSION)
	list(APPEND incs include/isp_frame)
	list(APPEND srcs
		src/isp_frame/isp_frame_session.c
		src/isp_frame/isp_frame_schema.c
		src/isp_frame/isp_frame_protocol.c
		src/isp_frame/isp_frame_stream.c
	)
	list(APPEND req bk_network_transfer json)
	list(APPEND priv_req multimedia_device_service media_service)
endif()

armino_component_register(INTERNAL_LIB SRCS "${srcs}" INCLUDE_DIRS "${incs}" REQUIRES ${req} PRIV_REQUIRES ${priv_req})

if (CONFIG_ISP_FRAME_SESSION)
	target_link_libraries(${COMPONENT_LIB} PRIVATE armino::bk_network_transfer armino::json)
endif()
