set(incs .)
set(srcs cp_main.c)

# CONFIG_VND_CAL + CONFIG_OVERRIDE_VND_CAL (board defconfig) require the
# project to supply vnd_cal (referenced by bk_init.c). Copied from app project.
if (CONFIG_VND_CAL)
if (CONFIG_OVERRIDE_VND_CAL)
    list(APPEND srcs
        vnd_cal.c
    )
endif()
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" REQUIRES tfm)
