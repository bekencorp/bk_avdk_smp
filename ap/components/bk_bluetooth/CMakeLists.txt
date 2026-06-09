set(incs)
set(srcs)

function(set_local_library_compile_option input_target)
    set(local_compile_option )

    set(TMP_WARNING_LIST)
    list(APPEND TMP_WARNING_LIST
                unused-but-set-variable
                unused-variable
                unused-function
                unused-but-set-variable
                unused-label
                #uninitialized
    )

    foreach(tmp_loopvar IN LISTS TMP_WARNING_LIST)
        list(APPEND local_compile_option -Wno-error=${tmp_loopvar} -Wno-${tmp_loopvar})
    endforeach()

    target_compile_options(${input_target} PRIVATE ${local_compile_option})
endfunction()

if (CONFIG_BLUETOOTH_AP)
    list(APPEND srcs
        api/bt_main.c
    )

    if (CONFIG_BLUETOOTH_SUPPORT_IPC)
        list(APPEND srcs
            ipc/src/bt_ipc_core.c
            ipc/src/bt_ipc_vendor_event_handler.c
        )
        list(APPEND incs
            ipc/include
        )
    endif()

	if (CONFIG_BLUETOOTH_HOST_ONLY OR CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL)
		if (CONFIG_BLUETOOTH_HOST_ONLY)
			list(APPEND srcs
				"hci/hal_hci_core.c"
				"hci/hci_distinguish.c"
				"hci/hci_parse.c"
				)
			list(APPEND incs
				hci
				)
		endif()

		if(CONFIG_AT)
			if(CONFIG_BT)
				if (CONFIG_BT_AT_ENABLE)
					list(APPEND srcs
						"api/bk_at_bt.c")
				endif()#CONFIG_BT_AT_ENABLE
			endif()#CONFIG_BT

			if (CONFIG_BLE_AT_ENABLE)
				list(APPEND srcs
					"api/bk_at_ble.c"
					"api/bk_at_gatt/bk_at_dm_performance_test.c"
				)
				# api/bk_at_gatt/dm_*.{c,h} have been replaced with shim headers
				# that forward bk_at_dm_* identifiers to the public bt_dm/ble/gatt
				# implementation. AT-layer translation units include the shim
				# headers directly via api/bk_at_gatt path.
				list(APPEND incs api/bk_at_gatt)
			endif()#CONFIG_BLE_AT_ENABLE

			set(components ../../components)
			list(APPEND incs
				${components}/at_server
				${components}/at_server/_at_server
				${components}/at_server/_at_server_port
			)
		endif()#CONFIG_AT

		if(CONFIG_ALI_MQTT)
			list(APPEND incs
				${components}/ali_mqtt/mqtt
				${components}/ali_mqtt/mqtt/utils
			)
		endif()#CONFIG_ALI_MQTT
		set(bk_inc ../../include)
			list(APPEND incs
				${bk_inc}/os
				#${components}/ali_mqtt/mqtt/utils
			)

		if (CONFIG_BT)
			list(APPEND srcs
				api/bt_l2cap_export_api.c
				api/bt_gap_export_api.c
				api/bk_a2dp_api.c
				api/bt_spp_export_api.c
				api/bt_hidd_export_api.c
				api/bt_hidh_export_api.c
				api/bk_hfp_api.c
				api/bk_pan_api.c
			)
		endif()

		if (CONFIG_BLE)
			list(APPEND srcs
				api/ble_gap_export_api.c
			)
		endif()

		list(APPEND incs
			include
			include/private
			.
			)

		if (CONFIG_BLUETOOTH_HOST_ONLY)
			list(APPEND srcs feat_config/bt_feature_config.c)

			if (CONFIG_SOC_BK7259)
				list(APPEND srcs
					soc/bk7259/bluetooth.c
					)
			endif()
		endif()

		if (CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL)
            list(APPEND srcs
                adapter/ble_ipc_client.c
                adapter/dm_ble_ipc_client.c
            )
            list(APPEND incs
                adapter
            )
		endif()

		if (CONFIG_BT AND CONFIG_BLUETOOTH_BTDM_COMPONENT_ENABLE)
			list(APPEND srcs
				bt_dm/utils/mpeg4_latm_dec.c
				bt_dm/utils/ring_buffer_node.c
				bt_dm/utils/ring_buffer_particle.c
				bt_dm/bt_manager/bt_manager.c
				bt_dm/storage/bluetooth_storage.c
				bt_dm/a2dp/bk_a2dp_sink_service.c
				bt_dm/avrcp/bk_avrcp_ct_service.c
				bt_dm/avrcp/bk_avrcp_tg_service.c
				)
			list(APPEND incs
				bt_dm/utils
				bt_dm/bt_manager
				bt_dm/storage
				bt_dm/a2dp
				bt_dm/avrcp
				)
		endif ()

		# BLE public GATT framework (formerly dashboard bluetooth_dual/gatt).
		# Built whenever BLE is enabled so that the AT layer (CONFIG_BLE_AT_ENABLE)
		# and external components (dashboard wifi_boarding) can link the public
		# dm_* symbols regardless of CONFIG_BLUETOOTH_BTDM_COMPONENT_ENABLE.
		if (CONFIG_BLE AND CONFIG_BLUETOOTH_BTDM_COMPONENT_BLE_GATT)
			list(APPEND srcs
				bt_dm/ble/gatt/dm_gatt.c
				bt_dm/ble/gatt/dm_gatts.c
				bt_dm/ble/gatt/dm_gattc.c
				bt_dm/ble/gatt/dm_gatt_connection.c
				bt_dm/ble/gatt/dm_gap_utils.c
				)
			if (CONFIG_CLI)
				list(APPEND srcs bt_dm/ble/gatt/dm_gatt_cli.c)
			endif()
			# Public include: downstream components that PRIV_REQUIRES bk_bluetooth
			# (e.g. dashboard bluetooth_dual/wifi_boarding) can directly
			# #include "dm_gatts.h" / "dm_gatt.h" etc.
			list(APPEND incs bt_dm/ble/gatt)
		endif()

		# HID-over-GATT keyboard demo, built on top of the public gatt framework.
		if (CONFIG_BLE AND CONFIG_BLUETOOTH_BTDM_COMPONENT_BLE_HOGPD)
			list(APPEND srcs bt_dm/ble/hogpd/hogpd_demo.c)
			if (CONFIG_CLI)
				list(APPEND srcs bt_dm/ble/hogpd/hogpd_demo_cli.c)
			endif()
			list(APPEND incs bt_dm/ble/hogpd)
		endif()
	endif()
endif()

if (CONFIG_CLI)
	list(APPEND srcs
		api/bk_cli_ble.c
	)
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES psa_mbedtls
	)

get_filename_component(config_component_name ${CMAKE_CURRENT_LIST_DIR} NAME)
armino_component_get_target(target "${config_component_name}")
set_local_library_compile_option(${target})

if (CONFIG_BLUETOOTH_AP)
    #host: <libbluetooth_host_ble|libbluetooth_host_dm_(bt|ble|dual)>[_nspe].a
    #controller: libbluetooth_controller_<bt|ble|dual|controller_only>[_<link num>link][_nspe].a

    set(bt_host_dm libbluetooth_host_dm)
    set(bt_host_ble libbluetooth_host_ble)

    set(bt_host_exist 0)
    set(bt_host_basename)

    if(CONFIG_BLUETOOTH_HOST_ONLY)
        set(bt_host_exist 1)
    endif()

    if(CONFIG_BLUETOOTH_HOST_ONLY)
		if(CONFIG_BLE AND CONFIG_BT)
			set(bt_host_basename ${bt_host_dm}_dual_ap)
		elseif(CONFIG_BLE)
			if(CONFIG_BLUETOOTH_BLE_SLAVE_ONLY)
				set(bt_host_basename ${bt_host_ble}_ap_slave_only)
			elseif(CONFIG_BLUETOOTH_BLE_SLAVE_OBSERVER)
				set(bt_host_basename ${bt_host_ble}_ap_slave_observer)
			else()
				set(bt_host_basename ${bt_host_ble}_ap)
			endif()
		endif()
	endif()

    set(bt_host_basename ${bt_host_basename}.a)

    set(bluetooth_elem)

    if(bt_host_exist EQUAL 1)
        list(APPEND bluetooth_elem ${bt_host_basename})
    endif()

    message("WARINIG bt_host_exist ${bt_host_exist}")
    message("WARINIG bt_host_basename ${bt_host_basename}")

	foreach(lib_name ${bluetooth_elem})
		#message("WARINIG lib_name ${ARMINO_PATH}/components/bk_libs/${ARMINO_SOC}/libs/${lib_name}")
		add_prebuilt_library(${lib_name} "${ARMINO_PATH}/components/bk_libs/${ARMINO_SOC}/libs/${lib_name}")
		target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
	endforeach()
endif()

armino_component_register_call_subdirs()
