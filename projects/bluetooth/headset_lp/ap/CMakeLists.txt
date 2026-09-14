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

# Share the AP demo sources with the sibling `headset` project. Only this
# project's config/<soc>/defconfig (+ Kconfig.projbuild) differs; all .c/.h are
# reused from ../../headset/ap so there is a single copy to maintain.
set(HS_AP ${CMAKE_CURRENT_LIST_DIR}/../../headset/ap)

set(incs . ${HS_AP})
set(srcs
	${HS_AP}/ap_main.c
	)

if (CONFIG_BT)
	list(APPEND incs
		${HS_AP}/hfp_hf
		${HS_AP}/a2dp_sink
	)

	list(APPEND srcs
		${HS_AP}/a2dp_sink/a2dp_sink_demo.c
		${HS_AP}/a2dp_sink_demo_cli.c
		${HS_AP}/hfp_hf/hfp_hf_demo.c
		)

	if (CONFIG_AUD_PARAM_CTRL)
		list(APPEND incs
			${HS_AP}/audio_param
		)
		list(APPEND srcs
			${HS_AP}/audio_param/bt_audio_param.c
			${HS_AP}/audio_param/a2dp_audio_para.c
			${HS_AP}/audio_param/hfp_audio_para.c
			)
	endif()
endif()

set(priv_req  bk_init media_service bk_cli audio_play audio_record lwip_intf_v2_1 bk_bluetooth)

if (CONFIG_AUD_PARAM_CTRL)
    list(APPEND priv_req bk_aud_param_ctrl)
endif()

if ("${ARMINO_SOC}" STREQUAL "bk7258")
list(APPEND priv_req)
endif()


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
get_filename_component(config_component_name ${CMAKE_CURRENT_LIST_DIR} NAME)
armino_component_get_target(target "${config_component_name}")
set_local_library_compile_option(${target})

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
