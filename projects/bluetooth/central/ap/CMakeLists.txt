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

set(incs .)
set(srcs
	ap_main.c
	)

if (CONFIG_BT)
    list(APPEND incs
        a2dp_source
    )

    list(APPEND srcs
        a2dp_source/ring_buffer_particle.c
        a2dp_source/a2dp_source_demo.c
        a2dp_source/a2dp_source_demo_cli.c
        a2dp_source/a2dp_source_demo_avrcp.c
        a2dp_source/a2dp_source_demo_calcu.c
    )
endif()

set(priv_req  bk_init media_service multimedia bk_cli audio_play audio_record lwip_intf_v2_1 bk_vfs)

if ("${ARMINO_SOC}" STREQUAL "bk7258")
list(APPEND priv_req)
endif()


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
get_filename_component(config_component_name ${CMAKE_CURRENT_LIST_DIR} NAME)
armino_component_get_target(target "${config_component_name}")
set_local_library_compile_option(${target})

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
