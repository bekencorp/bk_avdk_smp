set(incs .
    include)

set(srcs
	ap_main.c
	src/rlk_demo_cli.c
	src/rlk_ping.c
	src/rlk_iperf.c
	src/rlk_common.c
	)
	


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")
