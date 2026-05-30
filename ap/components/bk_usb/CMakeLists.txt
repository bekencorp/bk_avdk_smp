set(incs)
set(srcs)

if (CONFIG_USB)

	list(APPEND incs
		CherryUSB/
		CherryUSB/common
		CherryUSB/core
		CherryUSB/osal
		CherryUSB/class/cdc
		CherryUSB/class/hid
		CherryUSB/class/msc
		CherryUSB/class/audio
		CherryUSB/class/video
		CherryUSB/class/hub
		CherryUSB/demo
		CherryUSB/driver
	)
	list(APPEND srcs
		CherryUSB/driver/usb_driver.c
		CherryUSB/osal/usb_osal_adaptor.c
	)
	if (CONFIG_USB_DEVICE)
		# BK7259 USB device controller (MUSB-MHDRC) lives on a HIGH-SPEED
		# phy (SOC_USB_HS_BASE = 0x480A0000). The CherryUSB device port
		# usb_dc_beken_musb_mhdrc.c::usb_dc_init() gates the
		#   HWREGB(USB_BASE + MUSB_POWER_OFFSET) |= USB_POWER_HSENAB;
		# write behind #ifdef CONFIG_USB_HS; without that macro the
		# controller advertises USB 1.1 Full-Speed (mps=64) over a phy
		# that is still configured for HS (480 MHz), enumeration goes
		# sideways on the PC and the device shows up as "USB device not
		# recognised" or just hangs the AP cluster.
		#
		# The BK7258 SDK injected the macro via
		#   ap/components/bk_usb/CherryUSB/CMakeLists.txt:15
		#     sdk_compile_definitions(-DCONFIG_USB_HS)
		# but that file is gated by `if(CONFIG_CHERRYUSB_DEVICE)`, a
		# kconfig symbol that doesn't exist on this project (we use the
		# bk-armino-native `CONFIG_USB_DEVICE` instead). So on BK7259
		# the macro never reached the compile line and the device side
		# of USB ran in FS. We restore HS here unconditionally for all
		# BK7259 device-mode builds. See cmds/input.txt section 15 for
		# the full story of the BK7258 reference log that proved
		# mps=512 / HS is the right mode.
		add_compile_definitions(CONFIG_USB_HS)
		list(APPEND srcs
			CherryUSB/core/usbd_core.c
			CherryUSB/port/beken_musb/usb_dc_beken_musb_mhdrc.c
		)

		list(APPEND incs
			CherryUSB/demo/beken/usb_device
		)
		if (CONFIG_USBD_MSC)
			list(APPEND srcs
				CherryUSB/class/msc/usbd_msc.c
			)
			# usbd_msc.c references lv_vfs_init / lv_vfs_deinit
			# whenever CONFIG_VFS=y. The implementation file was
			# dropped from the BK7259 SDK, so we restore it here.
			# Without this the link fails with
			#   undefined reference to `lv_vfs_init` / `lv_vfs_deinit'
			# and the compile fails earlier still with
			#   fatal error: usb_vfs.h: No such file or directory
			if (CONFIG_VFS)
				list(APPEND srcs
					CherryUSB/class/msc/usb_vfs.c
				)
			endif() # CONFIG_VFS
		endif() # CONFIG_USBD_MSC

		if (CONFIG_USBD_UVC)
			list(APPEND srcs
				CherryUSB/class/video/usbd_video.c
				CherryUSB/demo/beken/usb_device/video/video_stream_common.c
			)
		endif() # CONFIG_USBD_UVC

		if (CONFIG_USBD_HID)
			list(APPEND srcs
				#CherryUSB/demo/beken/usb_device/hid/hid_common.c
				CherryUSB/demo/beken/usb_device/hid/hid_mouse_lpm.c
				CherryUSB/class/hid/usbd_hid.c
			)
		endif() # CONFIG_USBD_HID
	endif() # CONFIG_USB_DEVICE

	if (CONFIG_USB_HOST)
		list(APPEND incs
			CherryUSB/demo/beken/usb_host
		)
		list(APPEND srcs
			CherryUSB/core/usbh_core.c
			CherryUSB/port/beken_musb/usb_hc_beken_musb.c
		)

		if (CONFIG_USB_RISCV_BRIDGE)
			list(APPEND incs
				CherryUSB/port/beken_musb/riscv_bridge
			)
			list(APPEND srcs
				CherryUSB/port/beken_musb/riscv_bridge/riscv_usb_bridge.c
			)
		endif() # CONFIG_USB_RISCV_BRIDGE

		if (CONFIG_USB_HUB)
			list(APPEND srcs
				CherryUSB/class/hub/usbh_hub.c
			)
			if (CONFIG_USB_HUB_MULTIPLE_DEVICES)
				list(APPEND incs
					example/usbh_hub_multiple_classes
					)

					list(APPEND srcs
						example/usbh_hub_multiple_classes/bk_usbh_hub_multiple_classes_api.c
					)
			endif() # CONFIG_USB_HUB_MULTIPLE_DEVICES
		endif() # CONFIG_USB_HUB

		if (CONFIG_USBH_UVC)
			list(APPEND srcs
				CherryUSB/class/video/usbh_video.c
			)
		endif() # CONFIG_USBH_UVC

		if (CONFIG_USBH_UAC)
			list(APPEND srcs
				CherryUSB/class/audio/usbh_audio.c
			)
		endif() # CONFIG_USBH_UAC

		if (CONFIG_USBH_HID)
			list(APPEND srcs

			)
		endif() # CONFIG_USBH_HID

		if (CONFIG_USBH_MSC)
			list(APPEND srcs
				CherryUSB/class/msc/usbh_msc.c
			)
		endif() # CONFIG_USBH_MSC

		if (CONFIG_USB_CDC)
			list(APPEND srcs
				CherryUSB/class/cdc/usbh_cdc_acm.c
			)
		endif() # CONFIG_USB_CDC

		if (CONFIG_USBH_SERIAL_CH340)
			list(APPEND incs
				bk_usbh_simulate_uart/
			)
			list(APPEND srcs
				bk_usbh_simulate_uart/usbh_ch34x.c
				bk_usbh_simulate_uart/bk_usbh_simulate_uart_api.c
			)
		endif() # CONFIG_USBH_SIMULATE_SERIAL_CH340
    endif() # CONFIG_USB_HOST

	if (CONFIG_USB_DEVICE AND CONFIG_USB_HOST)
		list(APPEND srcs
			bk_usb_otg_manual_conversion.c
		)
	endif() # CONFIG_USB_DEVICE && CONFIG_USB_HOST

if (CONFIG_USB_CDC_MODEM)
    list (APPEND incs
        example/usb_cdc_acm
    )
    list (APPEND srcs
        example/usb_cdc_acm/bk_cherry_usb_cdc_acm_api.c
		example/usb_cdc_acm/bk_usb_cdc_modem.c
    )
endif() # CONFIG_USB_CDC_ACM_DEMO

endif() # CONFIG_USB

if (CONFIG_USB_CDC_ACM_DEMO)
    list (APPEND incs
        example/usb_cdc_acm
    )
    list (APPEND srcs
        example/usb_cdc_acm/bk_usb_cdc_demo.c
    )
endif() # CONFIG_USB_CDC_ACM_DEMO

if (CONFIG_ATE_TEST)
	list(APPEND incs
		example/ate_test
		)

	list(APPEND srcs
		example/ate_test/ate_test.c
		)
endif() # CONFIG_ATE

# bk_vfs is added to PRIV_REQUIRES so that CherryUSB/class/msc/usb_vfs.c
# can see bk_posix.h / bk_partition.h when CONFIG_USBD_MSC && CONFIG_VFS.
# Following the same pattern as bk_video_player / etc. that also need
# the VFS public headers.
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common driver bk_vfs)
