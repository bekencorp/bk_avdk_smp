#include "isp_camera_utils.h"


PIXEL_FORMAT_E isp_camera_format_convert(bk_pixel_format_t bk_format)
{

    switch (bk_format)
    {
        case BK_PIXEL_FORMAT_RGGB8:
            return PIXEL_FORMAT_RGGB8;
        case BK_PIXEL_FORMAT_GRBG8:
            return PIXEL_FORMAT_GRBG8;
        case BK_PIXEL_FORMAT_GBRG8:
            return PIXEL_FORMAT_GBRG8;
        case BK_PIXEL_FORMAT_BGGR8:
            return PIXEL_FORMAT_BGGR8;
        case BK_PIXEL_FORMAT_RAW8:
            return PIXEL_FORMAT_RAW8;

        case BK_PIXEL_FORMAT_RGGB10:
            return PIXEL_FORMAT_RGGB10;
        case BK_PIXEL_FORMAT_GRBG10:
            return PIXEL_FORMAT_RGGB10;
        case BK_PIXEL_FORMAT_GBRG10:
            return PIXEL_FORMAT_GBRG10;
        case BK_PIXEL_FORMAT_BGGR10:
            return PIXEL_FORMAT_BGGR10;
        case BK_PIXEL_FORMAT_RAW10:
            return PIXEL_FORMAT_RAW10;
        case BK_PIXEL_FORMAT_ARGB8565:
            return PIXEL_FORMAT_MAX;
        case BK_PIXEL_FORMAT_ABGR8565:
            return PIXEL_FORMAT_MAX;
        case BK_PIXEL_FORMAT_RGBA5658:
            return PIXEL_FORMAT_MAX;
        case BK_PIXEL_FORMAT_BGRA5658:
            return PIXEL_FORMAT_MAX;


        case BK_PIXEL_FORMAT_RGB888:
            return PIXEL_FORMAT_RGB888;
        case BK_PIXEL_FORMAT_BGR888:
            return PIXEL_FORMAT_MAX;

        case BK_PIXEL_FORMAT_ARGB8888:
            return PIXEL_FORMAT_MAX;
        case BK_PIXEL_FORMAT_ABGR8888:
            return PIXEL_FORMAT_MAX;
        case BK_PIXEL_FORMAT_RGBA8888:
            return PIXEL_FORMAT_MAX;
        case BK_PIXEL_FORMAT_BGRA8888:
            return PIXEL_FORMAT_MAX;


        case BK_PIXEL_FORMAT_NV12:
            return PIXEL_FORMAT_NV12;
        case BK_PIXEL_FORMAT_NV21:
            return PIXEL_FORMAT_NV21;
        case BK_PIXEL_FORMAT_YUYV:
            return PIXEL_FORMAT_YUYV;
        case BK_PIXEL_FORMAT_VYUY:
            return PIXEL_FORMAT_VYUY;
        case BK_PIXEL_FORMAT_UYVY:
            return PIXEL_FORMAT_UYVY;
        case BK_PIXEL_FORMAT_YYUV:
            return PIXEL_FORMAT_YYUV;

        default:
            return PIXEL_FORMAT_MAX;
    }

    return PIXEL_FORMAT_MAX;
}


