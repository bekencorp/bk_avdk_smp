set(srcs)
set(incs)

if (CONFIG_BLE_MESH_ZEPHYR)
    get_filename_component(SDK_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../../../../ ABSOLUTE)
    set(MESH_ROOT ${CMAKE_CURRENT_LIST_DIR})

    list(APPEND incs
        .
        ${SDK_ROOT}/ap/include/components/bluetooth
        ${SDK_ROOT}/ap/include/components
        ${SDK_ROOT}/ap/include
        ${SDK_ROOT}/ap/components/bk_bluetooth/core/ipc/include
        ${SDK_ROOT}/ap/components/bk_bluetooth/include/private
        ${MESH_ROOT}/include
        ${MESH_ROOT}/subsys/bluetooth
        ${MESH_ROOT}/sample
    )

    list(APPEND srcs
        hci_ipc_driver.c
        mesh_host.c

        ${MESH_ROOT}/subsys/bluetooth/common/log.c
        ${MESH_ROOT}/subsys/bluetooth/common/rpa.c

        ${MESH_ROOT}/subsys/net/net_buf.c

        ${MESH_ROOT}/subsys/sys/atomic_c.c
        ${MESH_ROOT}/subsys/sys/z_mem_slab.c
        ${MESH_ROOT}/subsys/sys/z_poll.c
        ${MESH_ROOT}/subsys/sys/z_queue.c
        ${MESH_ROOT}/subsys/sys/z_work.c
        ${MESH_ROOT}/subsys/sys/zephy_rtos.c
        ${MESH_ROOT}/subsys/sys/zephyr_work_thread.c

        ${MESH_ROOT}/subsys/tinycrypt/src/ble_aes_decrypt.c
        ${MESH_ROOT}/subsys/tinycrypt/src/ble_aes_encrypt.c
        ${MESH_ROOT}/subsys/tinycrypt/src/cbc_mode.c
        ${MESH_ROOT}/subsys/tinycrypt/src/ccm_mode.c
        ${MESH_ROOT}/subsys/tinycrypt/src/cmac_mode.c
        ${MESH_ROOT}/subsys/tinycrypt/src/ctr_mode.c
        ${MESH_ROOT}/subsys/tinycrypt/src/ctr_prng.c
        ${MESH_ROOT}/subsys/tinycrypt/src/ecc_dh.c
        ${MESH_ROOT}/subsys/tinycrypt/src/ecc_dsa.c
        ${MESH_ROOT}/subsys/tinycrypt/src/ecc_platform_specific.c
        ${MESH_ROOT}/subsys/tinycrypt/src/hmac_prng.c
        ${MESH_ROOT}/subsys/tinycrypt/src/ty_ecc.c
        ${MESH_ROOT}/subsys/tinycrypt/src/ty_hmac.c
        ${MESH_ROOT}/subsys/tinycrypt/src/ty_sha256.c
        ${MESH_ROOT}/subsys/tinycrypt/src/ty_utils.c

        ${MESH_ROOT}/subsys/bluetooth/host/uuid.c
        ${MESH_ROOT}/subsys/bluetooth/host/addr.c
        ${MESH_ROOT}/subsys/bluetooth/host/buf.c
        ${MESH_ROOT}/subsys/bluetooth/host/hci_core.c
        ${MESH_ROOT}/subsys/bluetooth/host/hci_common.c
        ${MESH_ROOT}/subsys/bluetooth/host/id.c
        ${MESH_ROOT}/subsys/bluetooth/host/adv.c
        ${MESH_ROOT}/subsys/bluetooth/host/scan.c
        ${MESH_ROOT}/subsys/bluetooth/host/ethermind_impl.c
        ${MESH_ROOT}/subsys/bluetooth/host/crypto.c
        ${MESH_ROOT}/subsys/bluetooth/host/ecc.c
        ${MESH_ROOT}/subsys/bluetooth/host/hci_driver.c

        ${MESH_ROOT}/subsys/bluetooth/mesh/main.c
        ${MESH_ROOT}/subsys/bluetooth/mesh/cfg.c
        ${MESH_ROOT}/subsys/bluetooth/mesh/adv.c
        ${MESH_ROOT}/subsys/bluetooth/mesh/beacon.c
        ${MESH_ROOT}/subsys/bluetooth/mesh/net.c
        ${MESH_ROOT}/subsys/bluetooth/mesh/subnet.c
        ${MESH_ROOT}/subsys/bluetooth/mesh/app_keys.c
        ${MESH_ROOT}/subsys/bluetooth/mesh/transport.c
        ${MESH_ROOT}/subsys/bluetooth/mesh/rpl.c
        ${MESH_ROOT}/subsys/bluetooth/mesh/heartbeat.c
        ${MESH_ROOT}/subsys/bluetooth/mesh/crypto.c
        ${MESH_ROOT}/subsys/bluetooth/mesh/access.c
        ${MESH_ROOT}/subsys/bluetooth/mesh/msg.c
        ${MESH_ROOT}/subsys/bluetooth/mesh/cfg_srv.c
        ${MESH_ROOT}/subsys/bluetooth/mesh/health_srv.c

        ${MESH_ROOT}/sample/ble_mesh_ui.c
        ${MESH_ROOT}/sample/ble_mesh_provision_sample.c
        ${MESH_ROOT}/sample/ble_mesh_led_sample.c
        ${MESH_ROOT}/sample/ble_mesh_tmall_spirit_sample.c
        ${MESH_ROOT}/sample/ble_mesh_export_api.c
    )

    if (CONFIG_CLI)
        list(APPEND srcs mesh_cli.c)
    endif()

    if (CONFIG_BT_MESH_ADV_LEGACY)
        list(APPEND srcs ${MESH_ROOT}/subsys/bluetooth/mesh/adv_legacy.c)
    endif()

    if (CONFIG_BT_MESH_ADV_EXT)
        list(APPEND srcs ${MESH_ROOT}/subsys/bluetooth/mesh/adv_ext.c)
    endif()

    if (CONFIG_BT_MESH_SETTINGS)
        list(APPEND srcs ${MESH_ROOT}/subsys/bluetooth/mesh/settings.c)
    endif()

    if (CONFIG_BT_MESH_LOW_POWER)
        list(APPEND srcs ${MESH_ROOT}/subsys/bluetooth/mesh/lpn.c)
    endif()

    if (CONFIG_BT_MESH_FRIEND)
        list(APPEND srcs ${MESH_ROOT}/subsys/bluetooth/mesh/friend.c)
    endif()

    if (CONFIG_BT_MESH_PROV)
        list(APPEND srcs ${MESH_ROOT}/subsys/bluetooth/mesh/prov.c)
    endif()

    if (CONFIG_BT_MESH_PROV_DEVICE)
        list(APPEND srcs ${MESH_ROOT}/subsys/bluetooth/mesh/prov_device.c)
    endif()

    if (CONFIG_BT_MESH_PROVISIONER)
        list(APPEND srcs ${MESH_ROOT}/subsys/bluetooth/mesh/provisioner.c)
    endif()

    if (CONFIG_BT_MESH_PB_ADV)
        list(APPEND srcs ${MESH_ROOT}/subsys/bluetooth/mesh/pb_adv.c)
    endif()

    if (CONFIG_BT_MESH_PB_GATT)
        list(APPEND srcs ${MESH_ROOT}/subsys/bluetooth/mesh/pb_gatt.c)
    endif()

    if (CONFIG_BT_MESH_GATT_SERVER)
        list(APPEND srcs ${MESH_ROOT}/subsys/bluetooth/mesh/gatt_services.c)
    endif()

    if (CONFIG_BT_MESH_GATT)
        list(APPEND srcs ${MESH_ROOT}/subsys/bluetooth/mesh/proxy_msg.c)
    endif()

    if (CONFIG_BT_MESH_CFG_CLI)
        list(APPEND srcs ${MESH_ROOT}/subsys/bluetooth/mesh/cfg_cli.c)
    endif()

    if (CONFIG_BT_MESH_HEALTH_CLI)
        list(APPEND srcs ${MESH_ROOT}/subsys/bluetooth/mesh/health_cli.c)
    endif()

    if (CONFIG_BT_MESH_SELF_TEST)
        list(APPEND srcs ${MESH_ROOT}/subsys/bluetooth/mesh/test.c)
    endif()

    if (CONFIG_BT_MESH_SHELL)
        list(APPEND srcs
            ${MESH_ROOT}/subsys/bluetooth/mesh/shell.c
            ${MESH_ROOT}/subsys/bluetooth/shell/bt.c
        )
    endif()

    if (CONFIG_BT_MESH_CDB)
        list(APPEND srcs ${MESH_ROOT}/subsys/bluetooth/mesh/cdb.c)
    endif()

    if (CONFIG_BT_MESH_CONN)
        list(APPEND srcs
            ${MESH_ROOT}/subsys/bluetooth/host/conn.c
            ${MESH_ROOT}/subsys/bluetooth/host/l2cap.c
            ${MESH_ROOT}/subsys/bluetooth/host/att.c
            ${MESH_ROOT}/subsys/bluetooth/host/gatt.c
        )
    endif()

    if (CONFIG_BT_MESH_SMP)
        list(APPEND srcs
            ${MESH_ROOT}/subsys/bluetooth/host/smp.c
            ${MESH_ROOT}/subsys/bluetooth/host/keys.c
        )
    else()
        list(APPEND srcs ${MESH_ROOT}/subsys/bluetooth/host/smp_null.c)
    endif()

    if (CONFIG_BT_MESH_ISO)
        list(APPEND srcs ${MESH_ROOT}/subsys/bluetooth/host/iso.c)
    endif()

    if (CONFIG_BT_MESH_HOST_CCM)
        list(APPEND srcs ${MESH_ROOT}/subsys/bluetooth/host/aes_ccm.c)
    endif()

    set(mesh_defs
        CONFIG_ACLSEMI_BT_MESH_LOOPBACK=0
        CONFIG_ACLSEMI_BTC_FEATURE_INCLUDED=1
        CONFIG_BLE_MESH_AP_HCI_IPC=1
    )

    if (CONFIG_BT_MESH_SETTINGS)
        list(APPEND mesh_defs CONFIG_BT_SETTINGS=1)
    endif()

    if (CONFIG_BT_MESH_SMP)
        list(APPEND mesh_defs CONFIG_BT_SMP=1)
    endif()

    if (CONFIG_BT_MESH_CONN)
        list(APPEND mesh_defs
            CONFIG_BT_CONN=1
            CONFIG_BT_GATT_DYNAMIC_DB=1
            CONFIG_BT_GATT_SERVICE_CHANGED=1
        )
    endif()

    if (CONFIG_BT_MESH_GATT_PROXY)
        list(APPEND mesh_defs CONFIG_BT_MESH_PROXY=1)
    endif()
endif()

armino_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS "${incs}"
    PRIV_REQUIRES bk_cli bk_bluetooth
)

if (CONFIG_BLE_MESH_ZEPHYR)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ${mesh_defs})
endif()
