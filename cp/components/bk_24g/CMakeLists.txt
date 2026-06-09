function(set_local_library_compile_option input_target)
    set(local_compile_option)

    set(TMP_WARNING_LIST)
    list(APPEND TMP_WARNING_LIST
        unused-but-set-variable
        unused-variable
        unused-function
        unused-but-set-variable
        unused-label

        # uninitialized
    )

    foreach(tmp_loopvar IN LISTS TMP_WARNING_LIST)
        list(APPEND local_compile_option -Wno-error=${tmp_loopvar} -Wno-${tmp_loopvar})
    endforeach()

    target_compile_options(${input_target} PRIVATE ${local_compile_option})
endfunction()

set(incs)
set(srcs)

if(CONFIG_BK24G)
    list(APPEND srcs

        # api/bt_main.c
    )

    list(APPEND incs
        include
        include/private
        .
    )

    list(APPEND srcs feat_config.c)

    if(CONFIG_SOC_BK7259)
        list(APPEND srcs
            soc/bk7259/soc.c
        )
    endif()
endif()

if(CONFIG_CLI)
    list(APPEND srcs
        bk_24g_cli.c
    )
endif()

armino_component_register(SRCS "${srcs}"
    INCLUDE_DIRS "${incs}"
    PRIV_REQUIRES bk_phy
)

if(CONFIG_BK24G)
    # #host: <libbluetooth_host_ble|libbluetooth_host_dm_(bt|ble|dual)>[_nspe].a
    # #controller: libbluetooth_controller_<bt|ble|dual|controller_only>[_<link num>link][_nspe].a

    # set(bt_host_dm libbluetooth_host_dm)
    # set(bt_host_ble libbluetooth_host_ble)
    # set(bt_controller libbluetooth_controller)

    # set(bt_host_exist 1)
    # set(bt_cp_exist 1)
    # set(bt_host_basename)
    # set(bt_controller_basename)

    # if(CONFIG_BTDM_CONTROLLER_ONLY)
    # set(bt_host_exist 0)
    # endif()

    # if(CONFIG_BTDM_CONTROLLER_ONLY)
    # if(CONFIG_BT)
    # set(bt_controller_basename ${bt_controller}_controller_only_dual)
    # else()
    # set(bt_controller_basename ${bt_controller}_controller_only_ble)
    # endif()
    # elseif(CONFIG_BT AND NOT CONFIG_BLE)
    # set(bt_host_basename ${bt_host_dm}_bt)
    # set(bt_controller_basename ${bt_controller}_bt)
    # elseif(CONFIG_BLE AND NOT CONFIG_BT)
    # set(bt_host_basename ${bt_host_ble})
    # set(bt_controller_basename ${bt_controller}_ble)
    # elseif(CONFIG_BLE AND CONFIG_BT)
    # set(bt_host_basename ${bt_host_dm}_dual)
    # set(bt_controller_basename ${bt_controller}_dual)
    # endif()

    # if(CONFIG_BLUETOOTH_RELEASE_CODESIZE AND CONFIG_BLE AND NOT CONFIG_BT)
    # set(bt_controller_basename ${bt_controller}_reduce)
    # set(bt_host_basename ${bt_host_ble}_reduce)
    # else()
    # if(CONFIG_BLUETOOTH_BLE_SLAVE_ONLY)
    # set(bt_controller_basename ${bt_controller}_ble_cp_slave_only)
    # if (NOT CONFIG_BTDM_CONTROLLER_ONLY)
    # set(bt_controller_basename ${bt_controller}_ble_slave_only)
    # set(bt_host_basename ${bt_host_ble}_slave_only)
    # endif()
    # elseif(CONFIG_BLUETOOTH_BLE_SLAVE_OBSERVER)
    # set(bt_controller_basename ${bt_controller}_ble_cp_slave_observer)
    # endif()
    # endif()

    # if (CONFIG_TZ AND NOT CONFIG_SPE)
    # #set(bt_host_basename ${bt_host_basename}_nspe)
    # set(bt_controller_basename ${bt_controller_basename}_nspe)
    # endif()

    # set(bt_host_basename ${bt_host_basename}.a)
    # set(bt_controller_basename ${bt_controller_basename}.a)

    # set(bluetooth_elem)

    # if(bt_host_exist EQUAL 1)
    # list(APPEND bluetooth_elem ${bt_host_basename})
    # endif()
    # if(bt_cp_exist EQUAL 1)
    # list(APPEND bluetooth_elem ${bt_controller_basename})
    # endif()

    # message("WARINIG bt_host_exist ${bt_host_exist}")
    # message("WARINIG bt_host_basename ${bt_host_basename}")
    # message("WARINIG bt_controller_basename ${bt_controller_basename}")

    # foreach(lib_name ${bluetooth_elem})
    # #message("WARINIG lib_name ${ARMINO_PATH}/components/bk_libs/${ARMINO_SOC}/libs/${lib_name}")
    # add_prebuilt_library(${lib_name} "${ARMINO_PATH}/components/bk_libs/${ARMINO_SOC}/libs/${lib_name}")
    # target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
    # endforeach()

    set(lib_name libbk24.a)
    add_prebuilt_library(${lib_name} "${ARMINO_PATH}/components/bk_libs/${ARMINO_SOC}/libs/${lib_name}")
    target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endif()

armino_component_register_call_subdirs()
