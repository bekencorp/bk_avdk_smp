

set(BK_PERIPHERAL_PATH $ENV{ARMINO_AP_DIR}/components/bk_peripheral)

set(GLOBAL_FUNCTION_SYMBOLS "")

include(${BK_PERIPHERAL_PATH}/src/lcd/rgb/config.cmake)
include(${BK_PERIPHERAL_PATH}/src/lcd/qspi/config.cmake)
include(${BK_PERIPHERAL_PATH}/src/lcd/spi/config.cmake)
include(${BK_PERIPHERAL_PATH}/src/lcd/dsi/config.cmake)
include(${BK_PERIPHERAL_PATH}/src/dvp/config.cmake)
include(${BK_PERIPHERAL_PATH}/src/tp/config.cmake)
include(${BK_PERIPHERAL_PATH}/src/csi/config.cmake)

set(incs include)
set(srcs
	src/bk_peripheral.c
	src/lcd/lcd_panel_devices.c
)

if (CONFIG_BK_PERIPHERAL)

list(APPEND srcs
	${RGB_LCD_DEVICE_FILES}
)

list(APPEND srcs
	${QSPI_LCD_DEVICE_FILES}
)

list(APPEND srcs
	${SPI_LCD_DEVICE_FILES}
)

list(APPEND srcs
	${DSI_LCD_DEVICE_FILES}
)

list(APPEND srcs
	${DVP_CAMERA_DEVICE_FILES}
)

list(APPEND srcs
	${CSI_CAMERA_DEVICE_FILES}
)

list(APPEND srcs
	${TP_DEVICE_FILES}
)
endif()

set(priv_req bk_common)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}" unity avdk_utils)

foreach(symbol ${GLOBAL_FUNCTION_SYMBOLS})
	target_link_libraries(${COMPONENT_LIB} INTERFACE "-u ${symbol}")
endforeach()

