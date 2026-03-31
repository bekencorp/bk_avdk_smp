/**
 * @file vg_lite_log.h
 *
 */

#ifndef VG_LITE_LOG_H
#define VG_LITE_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <os/os.h>


/*********************
 *      DEFINES
 *********************/
#define VG_LITE_TAG "VG"  /**< OS log tag*/

#define VG_LITE_LOG_TRACE(...)  BK_LOGI(VG_LITE_TAG, ##__VA_ARGS__)   /**< Output OS Info log */
#define VG_LITE_LOG_INFO(...)   BK_LOGI(VG_LITE_TAG, ##__VA_ARGS__)   /**< Output OS Info log */
#define VG_LITE_LOG_USER(...)   BK_LOGW(VG_LITE_TAG, ##__VA_ARGS__)   /**< Output OS Warning log */
#define VG_LITE_LOG_WARN(...)   BK_LOGW(VG_LITE_TAG, ##__VA_ARGS__)   /**< Output OS Warning log */
#define VG_LITE_LOG_ERROR(...)  BK_LOGE(VG_LITE_TAG, ##__VA_ARGS__)   /**< Output OS Error log */
#define VG_LITE_LOG(...)        BK_LOGD(VG_LITE_TAG, ##__VA_ARGS__)   /**< Output OS Debug log */

#endif /*VG_LITE_LOG_H*/
