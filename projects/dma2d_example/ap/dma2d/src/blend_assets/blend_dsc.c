#include "components/bk_draw_osd.h"
#include "blend.h"

const blend_info_t blend_assets[] =
{
    {.name = "wifi", .addr = &img_wifi_rssi0, .content = "wifi0"},
    {.name = "wifi", .addr = &img_wifi_rssi1, .content = "wifi1"},
    {.name = "wifi", .addr = &img_wifi_rssi2, .content = "wifi2"},
    {.name = "wifi", .addr = &img_wifi_rssi3, .content = "wifi3"},
    {.name = "wifi", .addr = &img_wifi_rssi4, .content = "wifi4"},
    {.name = "battery", .addr = &img_battery1, .content = "battery1"},
    {.name = "weather", .addr = &img_cloudy_to_sunny, .content = "cloudy_to_sunny"},
    {.addr = NULL},
};

const blend_info_t blend_info[] =
{
    {.name = "wifi", .addr = &img_wifi_rssi0, .content = "wifi0"},
    {.name = "battery", .addr = &img_battery1, .content = ""},
    {.name = "weather", .addr = &img_cloudy_to_sunny, .content = "cloudy_to_sunny"},
    {.addr = NULL},
};


