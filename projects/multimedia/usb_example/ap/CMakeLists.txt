set(incs .)

set(srcs ap_main.c uvc_test.c doorbell_uac_test.c)

set(priv_req avdk_utils bk_peripheral bk_cli bk_usb bk_uvc bk_voice_service fatfs)
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
