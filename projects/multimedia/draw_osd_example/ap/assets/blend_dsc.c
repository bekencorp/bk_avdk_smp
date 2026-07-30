#include "components/bk_draw_osd.h"
#include "blend.h"

const blend_info_t blend_assets[] =
{
    {.name = "clock", .addr = &font_clock, .content = "12:30"},
    {.name = "date",  .addr = &font_dates, .content = "2025年2月26日周三"},
    {.name = "ver",   .addr = &font_ver, .content = "v 1.0.0"},
    {.name = "text1" , .addr = &font_text1 , .content = "12:35" },
    {.name = "text2" , .addr = &font_text2 , .content = "博通集成欢迎您" },
    {.name = "beken_logo" , .addr = &img_img2 , .content = "beken_logo" },
    {.name = "img3" , .addr = &img_img3 , .content = "img3" },
    {.name = "wifi_group" , .addr = &wifi_rssi_none , .content = "wifi_rssi_none" },
    {.name = "wifi_group" , .addr = &wifi_rssi_1 , .content = "wifi_rssi_1" },
    {.name = "wifi_group" , .addr = &wifi_rssi_2 , .content = "wifi_rssi_2" },
    {.name = "wifi_group" , .addr = &wifi_rssi_3 , .content = "wifi_rssi_3" },
    {.name = "wifi_group" , .addr = &wifi_rssi_full , .content = "wifi_rssi_full" },
    {.name = "doggy" , .addr = &img_doggy , .content = "doggy" },
    {.addr = NULL},
};

const blend_info_t blend_info[] =
{
    {.name = "text1" , .addr = &font_text1 , .content = "12:00:00" },
    {.name = "text2" , .addr = &font_text2 , .content = "博通集成欢迎您" },
    {.name = "beken_logo" , .addr = &img_img2 , .content = "beken_logo" },
    {.name = "img3" , .addr = &img_img3 , .content = "img3" },
    {.name = "wifi_group" , .addr = &wifi_rssi_none , .content = "wifi_rssi_none" },
    {.addr = NULL},
};


