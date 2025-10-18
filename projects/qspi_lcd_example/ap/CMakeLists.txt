set(incs include  .)
set(srcs ap_main.c)

set(priv_req  media_service multimedia bk_display)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

