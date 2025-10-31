set(incs include  ./asr_service_test)
set(srcs ap_main.c)

list(APPEND srcs
    asr_service_test/cli_asr_service.c
)

set(priv_req media_service avdk_utils lwip_intf_v2_1 bk_wifi_driver bk_asr_service)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}"
)

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
