armino_build_get_property(target ARMINO_SOC)

armino_component_register(INCLUDE_DIRS .
	modules/securityip
	modules/freetype
	soc/${target}
)

target_include_directories(${COMPONENT_LIB} INTERFACE $ENV{SECURITY_PREBUILD_DIR})
