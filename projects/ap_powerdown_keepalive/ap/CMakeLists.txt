set(incs include  .
        lp_ipc_msg
        keepalive
)

set(srcs ap_main.c )

list(APPEND srcs
    lp_ipc_msg/lp_ipc_msg.c
    keepalive/keepalive.c
)

set(priv_req  bk_network_provisioning bk_network_transfer lwip_intf_v2_1 bk_wifi_driver)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
