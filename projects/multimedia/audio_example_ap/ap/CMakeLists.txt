set(incs .)
set(srcs ap_main.c)

#if (CONFIG_AUDIO)
list(APPEND srcs
    aud_test.c
    aud_test_data.c
)
#endif()
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
