set(incs .)

set(srcs
	ap_main.c
	usb_modem_at.c
	modem_at.c
	modem_at_cli.c
	modem_ecm_cfg.c
	net_check.c
	speed_test.c
	)

set(priv_req bk_usb bk_cli lwip_intf_v2_1)
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
