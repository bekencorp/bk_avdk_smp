#pragma once

#include "le_audio_user_config.h"

#define SMP_THREAD_USED                 (0)
#define BTA_PIPELINE                    (0)
#define CONFIG_WIFI_COEX_SCHEME         (0)
#define CONFIG_RECONN_INTERVAL          LE_AUDIO_RECONN_INTERVAL_MS

#define PAGE_SCAN_INTV                  LE_AUDIO_PAGE_SCAN_INTV
#define PAGE_SCAN_WIN                   LE_AUDIO_PAGE_SCAN_WIN
#define CONFIG_PAGE_TIMEOUT             LE_AUDIO_PAGE_TIMEOUT

/* Use audio_rsp resampler on BK7259 (7258 airplay used modules/src.h). */
#define BTA_RESAMPLE_USE_AUDIO_RSP      (1)
