if (CONFIG_PWM)
	list(APPEND incs pwm)
	list(APPEND incs pwm/v2p2)
	list(APPEND srcs pwm/v2p2/pwm_driver.c)

	if (CONFIG_CLI AND CONFIG_PWM_TEST)
		list(APPEND srcs pwm/pwm_test.c)
	endif()

	if (CONFIG_CLI AND CONFIG_PWM_API_TEST)
		list(APPEND srcs pwm/cli_pwm_api.c)
	endif()

	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()