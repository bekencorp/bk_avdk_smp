set(incs .)

set(srcs)

if (CONFIG_ADK_OPUS_ENC)

list(APPEND srcs
		opus_enc.c
	)

if (CONFIG_ADK_OPUS_ENC_TEST)
	list(APPEND srcs
		opus_enc_test.c
	)
endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common)