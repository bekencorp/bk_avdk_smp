set(incs .)
set(srcs
    ap_main.c
    flash_uart_stress_test.c
    )

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")
