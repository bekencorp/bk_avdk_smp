#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <driver/int.h>
#include <common/bk_err.h>
#include "box.h"

#include <avdk_check.h>

#include <components/bk_gpu.h>


#define TAG "box"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

extern bk_gpu_ctlr_handle_t app_gpu_handle_get(void);


int box_detection_path_build(FaceBox *faces, int count, int buffer_count, int rotate, int src_width, int src_height, int dst_width, int dst_height)
{
    uint32_t cmd_size;
    uint8_t  cmd_buff[buffer_count * 6 + 1];
    int16_t  dat_buff[buffer_count * 10];
    uint8_t* pcmd = cmd_buff;
    int16_t* pdat = dat_buff;

    float xscale = (float)dst_width / src_width;
    float yscale = (float)dst_height / src_width;

    if (count > 0)
    {
        for(int i = 0; i < count; i++)
        {
            int xmin = faces[i].xmin * xscale; if(xmin < 0) 	xmin = 0;
            int xmax = faces[i].xmax * xscale; if(xmax > dst_width) xmax = dst_width;
            int ymin = faces[i].ymin * yscale; if(ymin < 0)    ymin = 0;
            int ymax = faces[i].ymax * yscale; if(ymax > dst_height) ymax = dst_height;

            LOGI("box_detection_path_build: xmin=%d, ymin=%d, xmax=%d, ymax=%d\n", xmin, ymin, xmax, ymax);
            DRAW_RECTANGLE_PATH_BUILD(pcmd, pdat, xmin, ymin, xmax, ymax);
        }

        *pcmd = VLC_OP_END;
    }

    cmd_size = count * 6 + 1;

    bk_gpu_draw_path_set_t path_set = {
        .cmd = cmd_buff,
        .data = dat_buff,
        .size = cmd_size,
        .color = 0x00FF00
    };

    bk_gpu_draw_path_build(app_gpu_handle_get(), &path_set);

    return 0;
}

void box_detection_path_clear(void)
{
    bk_gpu_draw_path_clear(app_gpu_handle_get());
}