armino_build_get_property(target ARMINO_SOC)

string(REGEX REPLACE "_ap$" "" target_no_suffix ${target})

set(incs .)
list(APPEND incs 
	modules/veri_isp
	modules/vg_lite_gpu
	modules/private/veri_isp
	modules/private/vg_lite_gpu
	modules/fdk_aac_enc
	modules/securityip
	modules/freetype
	)
list(APPEND incs modules/dpu)

armino_component_register(INCLUDE_DIRS ${incs}
	soc/${target_no_suffix}
)

target_include_directories(${COMPONENT_LIB} INTERFACE $ENV{SECURITY_PREBUILD_DIR})
