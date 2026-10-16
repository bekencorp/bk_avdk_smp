set(incs .)

set(srcs
	ap_main.c
	usb_modem_at.c
	modem_at.c
	modem_at_cli.c
	)

set(priv_req bk_usb bk_cli)
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
