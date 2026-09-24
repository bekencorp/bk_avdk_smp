set(incs bk_private)
set(srcs)

if(CONFIG_MEDIA_SERVICE)

list(APPEND srcs
	osi_wrapper/audio_osi_wrapper.c
)

endif() #CONFIG_MEDIA_SERVICE

# CP hosts only the audio OSI wrapper. The flexa bond, isp, video and nano parts
# of the AP component have no CP counterpart, so the dependency list is trimmed
# to components that actually exist here. audio_osi_funcs_init() comes from the
# properties-side audio_osi library and is resolved at link time, exactly as on AP.
set(priv_req bk_common bk_rtos)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}" REQUIRES)
