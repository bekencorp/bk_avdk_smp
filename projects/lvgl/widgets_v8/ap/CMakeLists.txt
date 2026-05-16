set(incs .)

set(srcs
    ap_main.c
    )

list(APPEND srcs
    lv_demo_widgets.c
    assets/img_clothes.c
    assets/img_demo_widgets_avatar.c
    assets/img_lvgl_logo.c
)

set(priv_req  media_service bk_network_provisioning bk_display bk_peripheral bk_cli avdk_utils lvgl)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-variable -O3)
