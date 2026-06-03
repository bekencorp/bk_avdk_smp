set(incs .
         customer_msg
         doorbell_service
    )
set(srcs
    cp_main.c
    )

set(prev_req controller_if lwip_intf_v2_1 bt)

if (CONFIG_VND_CAL)
if (CONFIG_OVERRIDE_VND_CAL)
    list(APPEND srcs
        vnd_cal.c
    )
endif()
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${prev_req}")
