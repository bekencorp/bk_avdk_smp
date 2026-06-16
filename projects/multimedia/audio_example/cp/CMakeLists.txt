set(incs .)
set(srcs cp_main.c)
if (CONFIG_VND_CAL)
if (CONFIG_OVERRIDE_VND_CAL)
    list(APPEND srcs
        vnd_cal.c
    )
endif()
endif()


list(APPEND srcs
    aud_test.c
    aud_test_data.c
)

list(APPEND srcs
    aud_ate_test.c
)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")
