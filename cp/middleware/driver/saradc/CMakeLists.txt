if (CONFIG_SARADC)
	list(APPEND incs saradc)
	list(APPEND srcs saradc/adc_driver.c)

	if (CONFIG_ATE)
	list(APPEND srcs saradc/adc_ate.c)
	endif()

	if (CONFIG_CLI AND CONFIG_SARADC_TEST)
		list(APPEND srcs saradc/adc_test.c)
	endif()

	if (CONFIG_CLI AND CONFIG_SADC_API_TEST)
		list(APPEND srcs saradc/cli_sadc_api.c)
	endif ()

	if (CONFIG_CLI AND CONFIG_ADC_API_TEST)
		list(APPEND srcs saradc/cli_adc_api.c)
	endif()

	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()