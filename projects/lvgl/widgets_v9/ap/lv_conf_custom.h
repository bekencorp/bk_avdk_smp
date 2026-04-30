#ifndef PROJECT_LV_CONF_CUSTOM_H
#define PROJECT_LV_CONF_CUSTOM_H

/*
 * Project-level LVGL config.
 * Include the default component config first, then override only the
 * options needed by this project.
 */
#include "lv_conf.h"

#undef LV_USE_DEMO_WIDGETS
#define LV_USE_DEMO_WIDGETS 1

#undef LV_FONT_MONTSERRAT_24
#define LV_FONT_MONTSERRAT_24 1

#undef LV_FONT_MONTSERRAT_20
#define LV_FONT_MONTSERRAT_20 1

#undef LV_FONT_MONTSERRAT_18
#define LV_FONT_MONTSERRAT_18 1

#undef LV_FONT_MONTSERRAT_16
#define LV_FONT_MONTSERRAT_16 1

#undef LV_FONT_MONTSERRAT_12
#define LV_FONT_MONTSERRAT_12 1

#endif /* PROJECT_LV_CONF_CUSTOM_H */
