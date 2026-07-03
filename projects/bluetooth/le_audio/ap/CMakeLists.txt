function(set_local_library_compile_option input_target)
	set(local_compile_option)
	set(TMP_WARNING_LIST)
	list(APPEND TMP_WARNING_LIST
		unused-but-set-variable
		unused-variable
		unused-function
		unused-label
	)

	foreach(tmp_loopvar IN LISTS TMP_WARNING_LIST)
		list(APPEND local_compile_option -Wno-error=${tmp_loopvar} -Wno-${tmp_loopvar})
	endforeach()

	target_compile_options(${input_target} PRIVATE ${local_compile_option})
endfunction()

set(incs
	.
	bta
	bta/audio
	bta/ble
	bta/ring_buffer
	le_audio_demo
	le_audio_demo/sink/audio
	le_audio_demo/sink/common
)

set(srcs
	ap_main.c
	le_audio_demo_cli.c
	bta/bluetooth_app.c
	bta/bta_event.c
	bta/bta_manager.c
	bta/bta_cli.c
	bta/ring_buffer/bk_ring_buffer_node.c
	bta/ring_buffer/bk_ring_buffer_octets.c
	bta/ble/bta_auracast.c
	bta/audio/bta_audio.c
	bta/audio/bta_decode.c
	bta/audio/bta_encode.c
	bta/audio/bta_dac.c
	bta/audio/bta_resample.c
)

if (CONFIG_LE_AUDIO_PROTOCOL_DEMO)
	list(APPEND srcs
		le_audio_demo/le_audio_protocol_main.c
		le_audio_demo/source/broadcast/le_audio_broadcast_source.c
		le_audio_demo/source/unicast/le_audio_unicast_source.c
		le_audio_demo/sink/audio/le_audio_playback.c
		le_audio_demo/sink/common/le_audio_sink_core.c
		le_audio_demo/sink/common/le_audio_sink_media.c
		le_audio_demo/sink/broadcast/le_audio_broadcast_sink.c
		le_audio_demo/sink/unicast/le_audio_unicast_sink.c
	)
endif()

set(priv_req bk_init media_service bk_cli lwip_intf_v2_1 bk_bluetooth media_utils audio_play)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
get_filename_component(config_component_name ${CMAKE_CURRENT_LIST_DIR} NAME)
armino_component_get_target(target "${config_component_name}")
set_local_library_compile_option(${target})

file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
