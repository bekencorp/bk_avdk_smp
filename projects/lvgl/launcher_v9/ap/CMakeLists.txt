set(incs .)

# The UI can be built from JavaScript or from the native C implementation;
# ap_main.c selects the backend with LAUNCHER_UI_USE_JS. Native C and the
# JavaScript runtime are always linked, only the JavaScript source differs:
# the C bundle by default, the littlefs image when CONFIG_LAUNCHER_JS_FROM_FS
# is enabled.
set(srcs
    ap_main.c
    launcher.c
    game_2048.c
    juice_rush.c
    )

if(NOT CONFIG_LAUNCHER_JS_FROM_FS)
    list(APPEND srcs js_bundle.c)
endif()

set(priv_req  media_service bk_network_provisioning bk_display bk_peripheral bk_cli avdk_utils lvgl bk_jerryscript)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-variable -O3)
