set(incs . sdload)
set(srcs
    ap_main.c
    sdtest.c
    sdload/sdload.c
)

set(priv_req fatfs bk_cli)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
