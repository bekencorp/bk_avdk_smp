#ifndef _STUB_LOG_H_
#define _STUB_LOG_H_
#include <stdio.h>
#define BK_LOGI(tag, ...) do { printf("[I/%s] ", tag); printf(__VA_ARGS__); } while (0)
#define BK_LOGD(tag, ...) do { printf("[D/%s] ", tag); printf(__VA_ARGS__); } while (0)
#define BK_LOGE(tag, ...) do { printf("[E/%s] ", tag); printf(__VA_ARGS__); } while (0)
#endif
