set(incs .)
set(srcs
	ap_main.c
	dm_ble_gatt_server_demo.c
	)

armino_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_init bk_cli bk_bluetooth
)

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
