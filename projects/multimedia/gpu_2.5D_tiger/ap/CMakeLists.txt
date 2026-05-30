set(incs .)
set(srcs ap_main.c draw_tiger.c)

set(priv_req bk_network_provisioning bk_frame_buffer avdk_utils bk_gpu media_service bk_peripheral avdk_driver)

if(CONFIG_GCOV)
    list(APPEND priv_req embeded_gcov)
    list(APPEND incs ${ARMINO_PATH}/components/embeded_gcov/code)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

if(CONFIG_GCOV)
    component_compile_options(-fprofile-arcs -ftest-coverage -O0)
endif()
