#include <common/bk_include.h>
#include <components/log.h>
#include "audio_osi_wrapper.h"
#include "video_osi_wrapper.h"
#include "nano_osi_wrapper.h"
#include "isp_socket_wrapper.h"
#include "isp_i2c_wrapper.h"

#define TAG "media_sev"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

int media_service_init(void)
{
    bk_err_t ret = BK_OK;

    ret = bk_video_osi_funcs_init();
    
    if (ret != kNoErr)
    {
        LOGE("%s, bk_video_osi_funcs_init failed\n", __func__);
        return ret;
    }
    ret = bk_audio_osi_funcs_init();

    if (ret != kNoErr)
    {
        LOGE("%s, bk_audio_osi_funcs_init failed\n", __func__);
        return ret;
    }
    ret = bk_nano_osi_funcs_init();

    if (ret != BK_OK)
    {
        LOGE("%s failed\n", __func__);
        return ret;
    }

#if CONFIG_ISP
    ret = bk_isp_i2c_funcs_init();
    if (ret != BK_OK)
    {
        LOGE("%s, bk_isp_i2c_funcs_init failed\n", __func__);
        return ret;
    }
#endif
#ifdef CONFIG_LWIP_V2_1
    ret = bk_isp_socket_funcs_init();
    if (ret != BK_OK)
    {
        LOGE("%s, bk_isp_socket_funcs_init failed\n", __func__);
        return ret;
    }
#endif // CONFIG_LWIP_V2_1

    return 0;
}
