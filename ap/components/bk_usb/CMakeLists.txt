set(incs)
set(srcs)

if (CONFIG_USB)

	# Only the CherryUSB v1.6.x stack ships now; the legacy tree
	# (CherryUSB_legacy/) was removed. Keep using CONFIG_BK_USB_CHERRYUSB_V1_6
	# (forced y in Kconfig) for the per-feature guards below.
	set(BK_CHERRYUSB_DIR CherryUSB_v1_6)

	list(APPEND incs
		cherryusb_adapter/include
		bk_mtp
		${BK_CHERRYUSB_DIR}/
		${BK_CHERRYUSB_DIR}/common
		${BK_CHERRYUSB_DIR}/core
		${BK_CHERRYUSB_DIR}/osal
		${BK_CHERRYUSB_DIR}/class/cdc
		${BK_CHERRYUSB_DIR}/class/hid
		${BK_CHERRYUSB_DIR}/class/msc
		${BK_CHERRYUSB_DIR}/class/audio
		${BK_CHERRYUSB_DIR}/class/video
		${BK_CHERRYUSB_DIR}/class/hub
		${BK_CHERRYUSB_DIR}/demo
		${BK_CHERRYUSB_DIR}/driver
		../../include/components/cherryusb
	)
	if (CONFIG_BK_USB_CHERRYUSB_V1_6)
		list(APPEND incs
			${BK_CHERRYUSB_DIR}/port/musb
		)
	endif()
	list(APPEND srcs
		${BK_CHERRYUSB_DIR}/driver/usb_driver.c
		${BK_CHERRYUSB_DIR}/osal/usb_osal_adaptor.c
	)
	# Only the v1.6 adapter remains (legacy stack + its adapter were removed).
	list(APPEND srcs
		cherryusb_adapter/v1_6/bk_cherryusb_adapter_v1_6.c
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
		#   ap/components/bk_usb/${BK_CHERRYUSB_DIR}/CMakeLists.txt:15
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
			${BK_CHERRYUSB_DIR}/core/usbd_core.c
			${BK_CHERRYUSB_DIR}/port/beken_musb/usb_dc_beken_musb_mhdrc.c
		)

		# USB 2.0 device test mode (SET_FEATURE TEST_MODE -> MUSB TESTMODE reg).
		# CONFIG_USBDEV_TEST_MODE gates the device-core dispatch (usbd_core.c)
		# and the port implementation
		# (usb_dc_beken_musb_mhdrc.c::usbd_execute_test_mode). It also needs the
		# device-side CDC-ACM class driver used by the usb_hset CDC device;
		# it is pulled in here (rather than via CONFIG_USB_CDC, which would also
		# require the host usbh_cdc_acm.c that is not shipped in this SDK tree).
		# Opt-in via CONFIG_USB_HSET; harmless to existing device builds
		# when off. See ap/components/bk_usb/Kconfig.
		if (CONFIG_USB_HSET)
			add_compile_definitions(CONFIG_USBDEV_TEST_MODE)
			list(APPEND srcs
				${BK_CHERRYUSB_DIR}/class/cdc/usbd_cdc_acm.c
			)
		endif()

		list(APPEND incs
			${BK_CHERRYUSB_DIR}/demo/beken/usb_device
		)
		if (CONFIG_USBD_MSC)
			list(APPEND srcs
				${BK_CHERRYUSB_DIR}/class/msc/usbd_msc.c
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
					${BK_CHERRYUSB_DIR}/class/msc/usb_vfs.c
				)
			endif() # CONFIG_VFS
		endif() # CONFIG_USBD_MSC

		if (CONFIG_USBD_UVC)
			list(APPEND srcs
				${BK_CHERRYUSB_DIR}/class/video/usbd_video.c
				${BK_CHERRYUSB_DIR}/demo/beken/usb_device/video/video_stream_common.c
			)
		endif() # CONFIG_USBD_UVC

		if (CONFIG_USBD_HID)
			list(APPEND srcs
				#${BK_CHERRYUSB_DIR}/demo/beken/usb_device/hid/hid_common.c
				${BK_CHERRYUSB_DIR}/demo/beken/usb_device/hid/hid_mouse_lpm.c
				${BK_CHERRYUSB_DIR}/class/hid/usbd_hid.c
			)
		endif() # CONFIG_USBD_HID

		# MTP device class. CherryUSB v1.6 ships only the MTP protocol
		# headers (class/mtp/*.h) -- the engine .c is a commercial/closed
		# component upstream (see demo/mtp_template.c '#error commercial
		# charge'). We supply our own engine ported from the glass project
		# (ref/glass) and adapted to the v1.6 device API. It exposes a
		# mounted filesystem (bk_vfs POSIX) over MTP.
		if (CONFIG_USBD_MTP AND CONFIG_BK_USB_CHERRYUSB_V1_6)
			list(APPEND srcs
				bk_mtp/bk_usbd_mtp.c
			)
		endif() # CONFIG_USBD_MTP
	endif() # CONFIG_USB_DEVICE

	# RISC-V USB bridge plumbing is shared by the host AND device paths.
	# Pull it in once at the CONFIG_USB level so a device-only build (e.g.
	# usb_example/ap/config/bk7259_ap/defconfig with USB_DEVICE=y +
	# USBD_MSC=y) can also opt into CONFIG_USB_RISCV_BRIDGE without first
	# forcing CONFIG_USB_HOST=y. The host call site (usb_hc_beken_musb.c)
	# is itself still gated by CONFIG_USB_HOST below.
	if (CONFIG_USB_RISCV_BRIDGE)
		list(APPEND incs
			${BK_CHERRYUSB_DIR}/port/beken_musb/riscv_bridge
		)
		list(APPEND srcs
			${BK_CHERRYUSB_DIR}/port/beken_musb/riscv_bridge/riscv_usb_bridge.c
		)
	endif() # CONFIG_USB_RISCV_BRIDGE

	if (CONFIG_USB_HOST)
		list(APPEND incs
			${BK_CHERRYUSB_DIR}/demo/beken/usb_host
		)
		list(APPEND srcs
			${BK_CHERRYUSB_DIR}/core/usbh_core.c
			${BK_CHERRYUSB_DIR}/port/beken_musb/usb_hc_beken_musb.c
		)

		if (CONFIG_USB_HUB)
			list(APPEND srcs
				${BK_CHERRYUSB_DIR}/class/hub/usbh_hub.c
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
				${BK_CHERRYUSB_DIR}/class/video/usbh_video.c
			)
		endif() # CONFIG_USBH_UVC

		if (CONFIG_USBH_UAC)
			list(APPEND srcs
				${BK_CHERRYUSB_DIR}/class/audio/usbh_audio.c
			)
		endif() # CONFIG_USBH_UAC

		if (CONFIG_USBH_HID)
			list(APPEND srcs

			)
		endif() # CONFIG_USBH_HID

		if (CONFIG_USBH_MSC)
			list(APPEND srcs
				${BK_CHERRYUSB_DIR}/class/msc/usbh_msc.c
			)
		endif() # CONFIG_USBH_MSC

		if (CONFIG_USB_CDC)
			list(APPEND srcs
				${BK_CHERRYUSB_DIR}/class/cdc/usbh_cdc_acm.c
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

# bk_vfs is added to PRIV_REQUIRES so that ${BK_CHERRYUSB_DIR}/class/msc/usb_vfs.c
# can see bk_posix.h / bk_partition.h when CONFIG_USBD_MSC && CONFIG_VFS.
# Following the same pattern as bk_video_player / etc. that also need
# the VFS public headers.
# utf8 provides conv_utf8 / Utf8ToGb2312 used by bk_mtp/bk_usbd_mtp.c for
# GB2312<->UTF filename conversion when CONFIG_USBD_MTP && CONFIG_VFS.
set(usb_priv_req bk_common driver bk_vfs utf8)
if (CONFIG_USBD_MSC_STORAGE_QSPI_NAND)
	list(APPEND usb_priv_req dhara)
endif()
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${usb_priv_req}")
