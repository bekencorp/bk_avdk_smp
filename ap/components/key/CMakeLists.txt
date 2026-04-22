set(incs .)
set(srcs)

if (CONFIG_BUTTON)
	list(APPEND incs .
	)

	list(APPEND srcs
		key_main.c
		multi_button.c
		key_adapter.c
	)
endif() # CONFIG_BUTTON

set(priv_requires bk_common)
if (CONFIG_ADC_KEY)
	list(APPEND priv_requires adc_key)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES ${priv_requires})
