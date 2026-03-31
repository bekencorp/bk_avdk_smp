set(incs .)
set(srcs "")

list(APPEND srcs
    app_event.c
)

list(APPEND srcs
    bk_factory_config.c
)

set(priv_req bk_common bk_smart_config network_transfer bk_factory_config bk_led_blink bk_countdown bk_dual_screen_avi_player bk_vfs ota bk_batt_monitor)

armino_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS "${incs}"
    PRIV_REQUIRES "${priv_req}"
)