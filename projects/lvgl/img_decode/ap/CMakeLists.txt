set(incs .)

set(srcs
    ap_main.c
    )


set(priv_req  media_service multimedia)

if ("${ARMINO_SOC}" STREQUAL "bk7258_ap")
list(APPEND priv_req  lvgl bk_display bk_cli media_service)
endif()


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}"
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-variable -O3)

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)
