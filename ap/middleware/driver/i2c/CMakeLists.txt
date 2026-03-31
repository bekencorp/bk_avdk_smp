if (CONFIG_I2C)
	list(APPEND incs i2c)

	if (CONFIG_SIM_I2C)
		list(APPEND srcs
			i2c/sim_i2c_driver.c
			i2c/sim_i2c_driver_v2.c
		)
	else()
		list(APPEND srcs
			i2c/i2c_driver.c
			i2c/i2c_statis.c
		)
	endif()

	if (CONFIG_CLI AND CONFIG_I2C_TEST)
		list(APPEND srcs i2c/i2c_test.c)
	endif()
	
	if (CONFIG_CLI AND CONFIG_I2C_API_TEST)
		list(APPEND srcs i2c/cli_i2c_api.c)
	endif()


	set(incs ${incs} PARENT_SCOPE)
	set(srcs ${srcs} PARENT_SCOPE)
endif()