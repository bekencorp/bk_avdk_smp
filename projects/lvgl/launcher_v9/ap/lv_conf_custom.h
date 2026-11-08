#ifndef PROJECT_LV_CONF_CUSTOM_H
#define PROJECT_LV_CONF_CUSTOM_H

/*
 * Project-level LVGL config.
 * Include the default component config first, then override only the
 * options needed by this project.
 */
#include "lv_conf.h"

/* This project does not use the official LVGL Widgets demo; disable to save flash. */
#undef LV_USE_DEMO_WIDGETS
#define LV_USE_DEMO_WIDGETS 0

#undef LV_FONT_MONTSERRAT_24
#define LV_FONT_MONTSERRAT_24 1

#undef LV_FONT_MONTSERRAT_32
#define LV_FONT_MONTSERRAT_32 1

#undef LV_FONT_MONTSERRAT_20
#define LV_FONT_MONTSERRAT_20 1

#undef LV_FONT_MONTSERRAT_18
#define LV_FONT_MONTSERRAT_18 1

#undef LV_FONT_MONTSERRAT_16
#define LV_FONT_MONTSERRAT_16 1

#undef LV_FONT_MONTSERRAT_12
#define LV_FONT_MONTSERRAT_12 1

#undef LV_FONT_MONTSERRAT_40
#define LV_FONT_MONTSERRAT_40 1

#undef LV_FONT_MONTSERRAT_48
#define LV_FONT_MONTSERRAT_48 1

#undef LV_USE_NATIVE_HELIUM_ASM
#define LV_USE_NATIVE_HELIUM_ASM 1

#undef LV_USE_DRAW_SW_ASM
#define LV_USE_DRAW_SW_ASM LV_DRAW_SW_ASM_HELIUM

#endif /* PROJECT_LV_CONF_CUSTOM_H */
