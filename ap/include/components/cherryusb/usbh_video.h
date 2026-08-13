/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef USBH_VIDEO_H
#define USBH_VIDEO_H

#include "usb_video.h"
//#include <components/cherryusb/usb_video.h>
#include "usb_hc.h"

#define USBH_VIDEO_FORMAT_UNCOMPRESSED 0
#define USBH_VIDEO_FORMAT_MJPEG        1
#define USBH_VIDEO_FORMAT_H264         2
#define USBH_VIDEO_FORMAT_H265         3

#define USBH_VIDEO_FORMAT_MAX_NUM         5
#define USBH_VIDEO_FRAME_MAX_NUM          8

/* These MUST match the CherryUSB v1.6 host video driver (class/video/usbh_video.h)
 * so the public struct usbh_video is byte-identical to the objects it creates. */
#ifndef CONFIG_USBHOST_VIDEO_MAX_FRAMES
#define CONFIG_USBHOST_VIDEO_MAX_FRAMES 12
#endif
#ifndef CONFIG_USBHOST_VIDEO_MAX_FORMATS
#define CONFIG_USBHOST_VIDEO_MAX_FORMATS 3
#endif

struct usbh_video_resolution {
    uint16_t wWidth;
    uint16_t wHeight;
    uint16_t frame_index;
    uint16_t fps_num;
    uint32_t *fps;

};

struct usbh_video_format {
    struct usbh_video_resolution frame[CONFIG_USBHOST_VIDEO_MAX_FRAMES];
    uint8_t format_type;
    uint8_t num_of_frames;
};

struct usbh_videostreaming {
    uint8_t *bufbase;
    uint32_t bufoffset;
    void (*video_one_frame_callback)(struct usbh_videostreaming *stream);
};

/* MUST stay byte-identical to CherryUSB/class/video/usbh_video.h:
 * the v1.6 host driver creates these objects; external consumers read them.
 * The former v0.7 (pipe-based) and UVC_UAC_DEMO layouts were removed with the
 * legacy stack. */
struct usbh_video {
    struct usbh_hubport *hport;
    struct usb_endpoint_descriptor *isoin;  /* ISO IN endpoint */
    struct usb_endpoint_descriptor *bulkin; /* Bulk IN endpoint */

    uint8_t ctrl_intf; /* interface number */
    uint8_t data_intf; /* interface number */
    uint8_t minor;
    struct video_probe_and_commit_controls probe;
    struct video_probe_and_commit_controls commit;
    uint16_t isoin_mps;
    bool is_opened;
    uint8_t current_format;
    bool is_bulk;
    uint16_t bcdVDC;
    uint8_t num_of_intf_altsettings;
    uint8_t num_of_formats;
    struct usbh_video_format format[CONFIG_USBHOST_VIDEO_MAX_FORMATS];

    void *user_data;
};
#ifdef __cplusplus
extern "C" {
#endif

int usbh_video_get_cur(struct usbh_video *video_class, uint8_t intf, uint8_t entity_id, uint8_t cs, uint8_t *buf, uint16_t len);
int usbh_video_set_cur(struct usbh_video *video_class, uint8_t intf, uint8_t entity_id, uint8_t cs, uint8_t *buf, uint16_t len);
int usbh_videostreaming_get_cur_probe(struct usbh_video *video_class);
int usbh_videostreaming_set_cur_probe(struct usbh_video *video_class, uint8_t formatindex, uint8_t frameindex, uint32_t dwFrameInterval);
int usbh_videostreaming_set_cur_commit(struct usbh_video *video_class, uint8_t formatindex, uint8_t frameindex);
/* v1.6 host video open selects the format/frame by type+resolution internally. */
int usbh_video_open(struct usbh_video *video_class, uint8_t format_type, uint16_t wWidth, uint16_t wHeight, uint8_t altsetting);
int usbh_video_close(struct usbh_video *video_class);

void usbh_video_list_info(struct usbh_video *video_class);

void usbh_videostreaming_parse_mjpeg(struct usbh_urb *urb, struct usbh_videostreaming *stream);
void usbh_videostreaming_parse_yuyv2rgb565(struct usbh_urb *urb, struct usbh_videostreaming *stream);

void bk_usbh_video_sw_init(struct usbh_hubport *hport, uint8_t interface_num, uint8_t interface_sub_class);
void bk_usbh_video_sw_deinit( struct usbh_hubport *hport, uint8_t interface_num, uint8_t interface_sub_class);

void bk_usbh_video_unregister_dev(void);

void usbh_uvc_class_register();
#ifdef __cplusplus
}
#endif

#endif /* USBH_VIDEO_H */
