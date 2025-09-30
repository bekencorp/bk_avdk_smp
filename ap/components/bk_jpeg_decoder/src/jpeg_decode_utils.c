#include "components/media_types.h"
#include "components/avdk_utils/avdk_error.h"
#include "bk_jpeg_decode_ctlr.h"

avdk_err_t bk_get_jpeg_data_info(bk_jpeg_decode_img_info_t *img_info)
{

    uint8_t *src_buf;
    uint32_t length;
    if (img_info == NULL || img_info->frame == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    src_buf = img_info->frame->frame;
    length = img_info->frame->length;

    int flag = false;
    int i = 0;
    uint8_t *sof = NULL;
    uint8_t components = 0; // image color components count
    uint8_t simple_factor = 0; // image simple factor
    uint8_t msx = 0, msy = 0; // mcu block size
    uint32_t width = 0, height = 0;

    for (i = 0 ; i < length - 2;)
    {
        if (src_buf[i] == 0xFF)
        {
            if (src_buf[i + 1] == 0xC0)
            {
                flag = true;
                break;
            }
            else if (src_buf[i + 1] == 0xDA)
            {
                break;
            }
            else if (src_buf[i + 1] == 0xD8)
            {
                i += 2;
                continue;
            }
            else
            {
                int length = (src_buf[i + 2] << 8) | (src_buf[i + 3]);
                i += length + 2;
            }
        }
        else
        {
            i++;
        }
    }

    if (flag == false)
    {
        return AVDK_ERR_GENERIC;
    }

    sof = &src_buf[i];

    height = (sof[5] << 8) | (sof[6]);
    width = (sof[7] << 8) | (sof[8]);

    components = sof[9];
    if (components != 3 && components != 1)
    {
        return AVDK_ERR_UNSUPPORTED;
    }

    for (uint8_t k = 0; k < components; k++)
    {
        simple_factor = sof[11 + k * 3];
        if (k == 0)
        {
            // check simple factor
            if (simple_factor != 0x11 && simple_factor != 0x22 && simple_factor != 0x21)
            {
                return AVDK_ERR_UNSUPPORTED;
            }

            msx = simple_factor >> 4;
            msy = simple_factor & 0xF;
        }
        else
        {
            // check cb cr components
            if (simple_factor != 0x11)
            {
                return AVDK_ERR_UNSUPPORTED;
            }
        }
    }
    
    if (components == 1)
    {
        img_info->format = JPEG_FMT_YUV400;
    }
    else if (msx == 1)
    {
        img_info->format = JPEG_FMT_YUV444;
    }
    else if (msy == 1)
    {
        img_info->format = JPEG_FMT_YUV422;
    }
    else
    {
        img_info->format = JPEG_FMT_YUV420;
    }
    
    img_info->width = width;
    img_info->height = height;
    return AVDK_ERR_OK;
}