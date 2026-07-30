#ifndef __BLEND_H__
#define __BLEND_H__

#include "components/bk_draw_osd.h"

extern const bk_blend_t font_clock;
extern const bk_blend_t font_dates;
extern const bk_blend_t font_weather;
extern const bk_blend_t font_ver;
extern const bk_blend_t font_text1;     /* "12:35" time glyph */
extern const bk_blend_t font_text2;     /* CJK string glyph */
/* New OSD icons (RGBA raw data as arrays, 68px wide): wifi / weather(sun) / battery(charging) */
extern const bk_blend_t img_wifi0;      /* 68x76  wifi none */
extern const bk_blend_t img_wifi1;      /* 68x76  wifi 1 bar */
extern const bk_blend_t img_wifi2;      /* 68x76  wifi 2 bars */
extern const bk_blend_t img_wifi3;      /* 68x76  wifi 3 bars */
extern const bk_blend_t img_wifi4;      /* 68x76  wifi full */
extern const bk_blend_t img_sun;        /* 68x68  weather(sun) */
extern const bk_blend_t img_incharge;   /* 68x37  battery(charging) */
extern const bk_blend_t img_img1;   /* from assets/11.c: 244x244 ARGB8888 */
extern const bk_blend_t img_img2;   /* from assets/11.c: 212x212 ARGB8888 */
extern const bk_blend_t img_img3;
/* wifi_group: WiFi signal strength icon set (none/1/2/3/full, 60x60 ARGB8888) */
extern const bk_blend_t wifi_rssi_none;
extern const bk_blend_t wifi_rssi_1;
extern const bk_blend_t wifi_rssi_2;
extern const bk_blend_t wifi_rssi_3;
extern const bk_blend_t wifi_rssi_full;
extern const bk_blend_t img_doggy;

extern const blend_info_t blend_assets[];
extern const blend_info_t blend_info[];

#endif


