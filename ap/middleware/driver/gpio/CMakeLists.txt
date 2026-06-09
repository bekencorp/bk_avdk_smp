if (CONFIG_GPIO)
	list(APPEND incs gpio)
	if (CONFIG_ANA_GPIO)
		list(APPEND srcs gpio/ana_gpio_driver.c)
	endif()

	if (CONFIG_GPIO_CLOCK_PIN_SUPPORT)
		list(APPEND srcs gpio/gpio_clk_pin.c)
	endif()

	if (CONFIG_CLI AND CONFIG_GPIO_TEST)
		list(APPEND srcs gpio/gpio_test.c)
	endif()

	if (CONFIG_GPIO_V2PX)
		list(APPEND incs gpio/v2px)
		list(APPEND srcs gpio/v2px/gpio_driver_base.c)
	endif()
endif()

set(incs ${incs} PARENT_SCOPE)
set(srcs ${srcs} PARENT_SCOPE)
