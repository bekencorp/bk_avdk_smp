set(incs .)
set(srcs "")

list(APPEND srcs
	key_app_service.c
)


armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES bk_common media_service multimedia bk_vfs 
				  bk_app_event key ap bk_smart_config  bk_factory_config
				  )