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

set(common_dir ${CMAKE_CURRENT_LIST_DIR}/../../common)
set(incs . ${common_dir})
set(srcs
	ap_main.c
	unicast_client_demo.c
	unicast_client_cli.c
	${common_dir}/audio_codec.c
	${common_dir}/audio_tx.c
	${common_dir}/audio_tone.c
 )
set(priv_req bk_init bk_bluetooth bk_cli)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
get_filename_component(config_component_name ${CMAKE_CURRENT_LIST_DIR} NAME)
armino_component_get_target(target "${config_component_name}")
set_local_library_compile_option(${target})

file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
