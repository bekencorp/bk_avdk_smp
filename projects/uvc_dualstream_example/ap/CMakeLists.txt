set(incs . uvc_test/include)
set(srcs ap_main.c)

if (CONFIG_USB_CAMERA)
    list(APPEND srcs
        uvc_test/src/uvc_main.c
        uvc_test/src/uvc_common.c
    )
endif()

set(priv_req  media_service bk_camera bk_cli)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)