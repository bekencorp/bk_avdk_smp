set(incs .)
set(srcs cp_main.c)

list(APPEND incs
    2.4g
)

list(APPEND srcs
    2.4g/2.4g_demo.c
    2.4g/2.4g_demo_cli.c
)

if (CONFIG_VND_CAL)
if (CONFIG_OVERRIDE_VND_CAL)
    list(APPEND srcs
        vnd_cal.c
    )
endif()
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common include bk_24g)
