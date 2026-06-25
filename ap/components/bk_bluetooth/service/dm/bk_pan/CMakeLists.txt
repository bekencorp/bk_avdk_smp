set(incs)
set(srcs)

if (CONFIG_NET_PAN)
    list(APPEND incs
        .
    )

    list(APPEND srcs
        pan_service.c
        pan_demo_cli.c
        bt_comm_list.c
        hidd_service.c
    )

    if (NOT CONFIG_BLUETOOTH_BTDM_COMPONENT_ENABLE)
        list(APPEND incs
            ../bt_manager
            ../storage
        )

        list(APPEND srcs
            ../bt_manager/bt_manager.c
            ../storage/bluetooth_storage.c
        )
    endif()
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" 
    PRIV_REQUIRES lwip_intf_v2_1 bk_bluetooth)

