set(incs .)
set(srcs cp_main.c)

list(APPEND srcs
    aud_test.c
    aud_test_data.c
)

list(APPEND srcs
    aud_ate_test.c
)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")
