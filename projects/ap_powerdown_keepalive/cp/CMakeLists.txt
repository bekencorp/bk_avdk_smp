set(incs .
         lp_ipc_msg
         keepalive
         powerctrl
)

set(srcs
    cp_main.c
)


list(APPEND srcs
    lp_ipc_msg/lp_ipc_msg.c
    keepalive/keepalive_msg.c
    keepalive/keepalive_client.c
    keepalive/keepalive_server.c
    powerctrl/powerctrl.c
)

set(prev_req controller_if lwip_intf_v2_1 bt)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${prev_req}")
