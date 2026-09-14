# Share the CP sources with the sibling `headset` project; only this project's
# config/<soc>/defconfig differs. See ap/CMakeLists.txt for the rationale.
set(HS_CP ${CMAKE_CURRENT_LIST_DIR}/../../headset/cp)

set(incs . ${HS_CP})
set(srcs
    ${HS_CP}/cp_main.c
    )
if (CONFIG_VND_CAL)
if (CONFIG_OVERRIDE_VND_CAL)
    list(APPEND srcs
        vnd_cal.c
    )
endif()
endif()


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")
