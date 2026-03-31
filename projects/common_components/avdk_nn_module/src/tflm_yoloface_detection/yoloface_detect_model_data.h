#pragma once

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

/* C 静态初始化需要常量，此处用宏 */
#define YOLOFACE_MODEL_DATA_SIZE 34416
extern const unsigned char g_yoloface_int8_vela_tflite[];

#ifdef  __cplusplus
}
#endif//__cplusplus
