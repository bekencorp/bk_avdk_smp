set(incs)
set(srcs)
if (CONFIG_USB_CAMERA)
	list(APPEND incs . include)

	list(APPEND srcs
		src/bk_uvc.c
		src/bk_uvc_stream.c
		src/bk_camera_uvc_ctlr.c
		src/uvc_urb_list.c
	)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common bk_usb)