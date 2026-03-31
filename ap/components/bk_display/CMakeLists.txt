set(incs include)
set(srcs "")

if (CONFIG_BK_DISPLAY)

	list(APPEND srcs
		src/bk_display.c
		src/bk_display_bus.c
		src/dpu/bk_display_dpu_vn_default.c

		src/bus/bk_display_rgb_bus.c
		src/bus/bk_display_i2c_bus.c
		src/bus/lcd_rgb_panel_common.c
		src/panel/bk_lcd_panel.c

	)

	if (CONFIG_MIPI_DSI)
		list(APPEND srcs
			src/bus/bk_display_dsi_bus.c
			src/bus/lcd_mipi_panel_common.c
		)
	endif()

	if (CONFIG_LCD_SPI)
		list(APPEND srcs
			src/bus/bk_display_spi_bus.c
		)
	endif()
endif()

set(priv_req bk_common unity include bk_peripheral)

armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES "${priv_req}"  avdk_utils avdk_driver
)

