#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <common/bk_assert.h>
#include <avdk_check.h>

#include <common/avdk_pixel_types.h>

uint32_t bk_pixel_size_get(bk_pixel_format_t format)
{
    switch (format)
    {
	case BK_PIXEL_FORMAT_YUYV:
	case BK_PIXEL_FORMAT_NV12:
	case BK_PIXEL_FORMAT_NV21:
        return 2; // TODO
	case BK_PIXEL_FORMAT_RGB565:
	case BK_PIXEL_FORMAT_BGR565:
        return 2;
	case BK_PIXEL_FORMAT_RGB888:
	case BK_PIXEL_FORMAT_BGR888:
    case BK_PIXEL_FORMAT_ARGB8565:
    case BK_PIXEL_FORMAT_ABGR8565:
    case BK_PIXEL_FORMAT_RGBA5658:
    case BK_PIXEL_FORMAT_BGRA5658:
        return 3;
	case BK_PIXEL_FORMAT_ARGB8888:
    case BK_PIXEL_FORMAT_ABGR8888:
    case BK_PIXEL_FORMAT_RGBA8888:
    case BK_PIXEL_FORMAT_BGRA8888:
        return 4;
    default:
        return 0;
    }
    return 0;
}

uint32_t bk_image_size_get(uint16_t width, uint16_t height, bk_pixel_format_t format)
{
    switch (format)
    {

        case BK_PIXEL_FORMAT_BGGR8:
        case BK_PIXEL_FORMAT_GBRG8:
        case BK_PIXEL_FORMAT_GRBG8:
        case BK_PIXEL_FORMAT_RGGB8:
        case BK_PIXEL_FORMAT_RAW8:
            return width * height;

        case BK_PIXEL_FORMAT_BGGR10:
        case BK_PIXEL_FORMAT_GBRG10:
        case BK_PIXEL_FORMAT_GRBG10:
        case BK_PIXEL_FORMAT_RGGB10:
        case BK_PIXEL_FORMAT_RAW10:
            return width * height * 2;

        case BK_PIXEL_FORMAT_RGB565:
        case BK_PIXEL_FORMAT_BGR565:
            return width * height * 2;

        case BK_PIXEL_FORMAT_ARGB8565:
        case BK_PIXEL_FORMAT_ABGR8565:
        case BK_PIXEL_FORMAT_RGBA5658:
        case BK_PIXEL_FORMAT_BGRA5658:
        case BK_PIXEL_FORMAT_RGB888:
        case BK_PIXEL_FORMAT_BGR888:
            return width * height * 3;

        case BK_PIXEL_FORMAT_ARGB8888:
        case BK_PIXEL_FORMAT_ABGR8888:
        case BK_PIXEL_FORMAT_RGBA8888:
        case BK_PIXEL_FORMAT_BGRA8888:
            return width * height * 4;

        case BK_PIXEL_FORMAT_NV12:
        case BK_PIXEL_FORMAT_NV21:
            return width * height * 3 / 2;

        case BK_PIXEL_FORMAT_YUYV:
        case BK_PIXEL_FORMAT_VYUY:
        case BK_PIXEL_FORMAT_UYVY:
        case BK_PIXEL_FORMAT_YYUV:
            return width * height * 2;

        default:
            return 0;
    }
}