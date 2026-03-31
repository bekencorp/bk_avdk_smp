set(incs include)
set(srcs
    src/psram_dma_stress.c
)
set(priv_req bk_common driver)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

