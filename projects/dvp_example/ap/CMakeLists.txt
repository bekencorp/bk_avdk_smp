set(incs . dvp_test/include)
set(srcs ap_main.c)

set(GLOBAL_FUNCTION_SYMBOLS "")

if (CONFIG_DVP_CAMERA)
    list(APPEND srcs
        dvp_test/src/dvp_main.c
        dvp_test/src/dvp_func_test.c
        dvp_test/src/dvp_api_test.c
        dvp_test/src/dvp_frame_list.c
        dvp_test/src/dvp_gc0001_test.c
    )

    list(APPEND GLOBAL_FUNCTION_SYMBOLS "gc0001_detect_sensor")
endif()

set(priv_req  media_service multimedia bk_camera bk_cli)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

foreach(symbol ${GLOBAL_FUNCTION_SYMBOLS})
	target_link_libraries(${COMPONENT_LIB} INTERFACE "-u ${symbol}")
endforeach()

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)