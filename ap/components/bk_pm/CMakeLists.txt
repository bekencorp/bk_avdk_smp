armino_build_get_property(target ARMINO_SOC)
set(incs include)

set(srcs src/bk_pm.c)

if (CONFIG_PM_ENABLE)
	if (CONFIG_PM_CLIENT)
		list(APPEND srcs
			src/clients/bk_pm_client.c
			src/clients/bk_pm_client_power.c
			src/clients/bk_pm_client_clock.c
			src/clients/bk_pm_client_mailbox.c
			src/clients/bk_pm_client_sleep.c
			src/clients/bk_pm_client_ap_ctrl.c
			src/clients/bk_pm_client_thread.c
			src/clients/bk_pm_client_misc.c
		)
	endif()

endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
