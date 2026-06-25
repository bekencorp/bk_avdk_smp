set(incs .
         lp_ipc_msg
         keepalive
         powerctrl
)

set(srcs
    cp_main.c
)
if (CONFIG_VND_CAL)
if (CONFIG_OVERRIDE_VND_CAL)
    list(APPEND srcs
        vnd_cal.c
    )
endif()
endif()



list(APPEND srcs
    lp_ipc_msg/lp_ipc_msg.c
    keepalive/keepalive_msg.c
    keepalive/keepalive_client.c
    keepalive/keepalive_server.c
    powerctrl/powerctrl.c
)

set(prev_req controller_if lwip_intf_v2_1 bk_bluetooth)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${prev_req}")
