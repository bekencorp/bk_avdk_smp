set(incs .)
set(srcs
    cp_main.c
    )

# if (CONFIG_OVERRIDE_FLASH_PARTITION)
#     list(APPEND srcs
#         ../partitions/vendor_flash.c
#     )
# endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}")
