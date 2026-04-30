set(incs . aec_m52)
set(srcs)
set(priv_requires bk_common driver)

if (CONFIG_AEC_RUN_ON_M52)
	list(APPEND srcs
		aec_m52/audio_mp52_engine.c
		aec_m52/audio_mp52_ipc_cp.c
		aec_m52/audio_mp52_module_init.c
	)
endif()

armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES ${priv_requires}
	)

