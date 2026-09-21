set(incs
	.
	aec_m52
	audio_pipeline
	audio_streams/raw_stream
	audio_streams/array_stream
	audio_streams/onboard_mic_stream
	audio_streams/onboard_mic_stream/v1.0
	audio_streams/onboard_mic_stream/v2.0
	audio_streams/onboard_speaker_stream
	audio_streams/onboard_speaker_stream/v2.0
	audio_encoders/g711_encoder
	audio_decoders/g711_decoder
	audio_decoders/mp3_decoder
	audio_decoders/wav_decoder
	audio_algorithms/aec_v3_algorithm
	audio_algorithms/aec_v3_algorithm/v2.0
	audio_algorithms/eq_algorithm
	audio_utils
)
set(srcs)
set(priv_requires bk_common driver)

if (CONFIG_ADK)
	list(APPEND srcs
		audio_osi_wrapper.c
	)
	list(APPEND priv_requires audio_pipeline audio_osi)
endif()

if (CONFIG_ADK_RAW_STREAM)
	list(APPEND priv_requires raw_stream)
endif()

if (CONFIG_ADK_ARRAY_STREAM)
	list(APPEND priv_requires array_stream)
endif()

if (CONFIG_ADK_ONBOARD_MIC_STREAM)
	list(APPEND priv_requires onboard_mic_stream)
endif()

if (CONFIG_ADK_ONBOARD_SPEAKER_STREAM)
	list(APPEND priv_requires onboard_speaker_stream)
endif()

if (CONFIG_ADK_G711_DECODER)
	list(APPEND priv_requires g711_decoder)
endif()

if (CONFIG_ADK_MP3_DECODER)
	list(APPEND priv_requires mp3_decoder helix)
endif()

if (CONFIG_ADK_WAV_DECODER)
	list(APPEND priv_requires wav_decoder wav)
endif()

if (CONFIG_ADK_G711_ENCODER)
	list(APPEND priv_requires g711_encoder)
endif()

if (CONFIG_ADK_G711_DECODER OR CONFIG_ADK_G711_ENCODER)
	list(APPEND priv_requires g711)
endif()

if (CONFIG_ADK_AEC_V3_ALGORITHM)
	list(APPEND priv_requires aec_v3_algorithm)
endif()

if (CONFIG_ADK_EQ_ALGORITHM)
	list(APPEND priv_requires eq_algorithm eq)
endif()

if (CONFIG_ADK_COUNT_UTIL)
	list(APPEND priv_requires audio_utils)
endif()

if (CONFIG_ADK_OBS_UTIL)
	list(APPEND priv_requires audio_utils)
endif()

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

armino_component_register_call_subdirs()
