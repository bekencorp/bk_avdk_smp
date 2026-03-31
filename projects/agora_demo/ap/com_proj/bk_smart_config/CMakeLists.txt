set(incs . include)
set(srcs)

if (CONFIG_BK_SMART_CONFIG)
list(APPEND srcs
    src/core/bk_smart_config_core.c
)

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" 
    REQUIRES bk_network_provisioning
    PRIV_REQUIRES bk_network_provisioning network_transfer 
                  bk_common json lwip_intf_v2_1 
                  bk_uid bk_pan bk_app_event bk_factory_config 
	)