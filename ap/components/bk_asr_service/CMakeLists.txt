set(incs include)
set(srcs "")

if (CONFIG_ASR_SERVICE)
	list(APPEND srcs
		src/bk_asr.c
		src/bk_asr_service.c
	)

if (CONFIG_WANSON_ASR)

	list(APPEND incs
		wanson_doorbell
	)

	list(APPEND srcs
		wanson_doorbell/bk_wanson_doorbell_asr_intf.c
	)
endif()


if (CONFIG_WANSON_ARMINO_ASR)

	list(APPEND incs
		wanson_armino
	)

	list(APPEND srcs
		wanson_armino/bk_wanson_armino_asr_intf.c
	)
endif()


endif()

armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES avdk_utils bk_common include wanson wanson_armino
)
