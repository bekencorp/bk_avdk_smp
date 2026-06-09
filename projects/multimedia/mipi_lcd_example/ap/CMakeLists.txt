set(incs . include )

set(srcs 
	ap_main.c
	src/lcd_example_mipi.c
	src/lcd_stress_test.c
)

set(priv_req  media_service bk_network_provisioning  bk_cli avdk_utils bk_peripheral)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
