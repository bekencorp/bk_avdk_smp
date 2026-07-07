/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef USBH_AUDIO_H
#define USBH_AUDIO_H

#include "usb_audio.h"
#include "usb_config.h"
#include "usb_hc.h"

#if CONFIG_BK_USB_CHERRYUSB_V1_6
/* v1.6 host audio uses a different, descriptor-oriented layout than the legacy
 * driver. struct usbh_audio is allocated by the (separately compiled) CherryUSB
 * v1.6 library, so this MUST stay byte-for-byte identical to
 * CherryUSB_v1_6/class/audio/usbh_audio.h, otherwise BK glue code (which reads
 * these fields through the pointer) would read at the wrong offsets. */
#include "usb_audio.h"

#ifndef CONFIG_USBHOST_AUDIO_MAX_STREAMS
#define CONFIG_USBHOST_AUDIO_MAX_STREAMS 3
#endif

struct usbh_audio_ac_msg {
    struct audio_cs_if_ac_input_terminal_descriptor ac_input;
    struct audio_cs_if_ac_feature_unit_descriptor ac_feature_unit;
    struct audio_cs_if_ac_output_terminal_descriptor ac_output;
};

struct usbh_audio_as_msg {
    const char *stream_name;
    uint8_t stream_intf;
    uint8_t input_terminal_id;
    uint8_t feature_terminal_id;
    uint8_t output_terminal_id;
    uint8_t ep_attr;
    uint8_t num_of_altsetting;
    uint16_t volume_min;
    uint16_t volume_max;
    uint16_t volume_res;
    uint16_t volume_cur;
    bool mute;
    struct audio_cs_if_as_general_descriptor as_general;
    struct audio_cs_if_as_format_type_descriptor as_format[CONFIG_USBHOST_MAX_INTF_ALTSETTINGS];
};

struct usbh_audio {
    struct usbh_hubport *hport;
    struct usb_endpoint_descriptor *isoin;  /* ISO IN endpoint */
    struct usb_endpoint_descriptor *isoout; /* ISO OUT endpoint */

    uint8_t ctrl_intf; /* interface number */
    uint8_t minor;
    uint16_t isoin_mps;
    uint16_t isoout_mps;
    bool is_opened;
    uint16_t bcdADC;
    uint8_t bInCollection;
    uint8_t stream_intf_num;
    struct usbh_audio_as_msg as_msg_table[CONFIG_USBHOST_AUDIO_MAX_STREAMS];

    void *user_data;
};
#else
struct usbh_audio_format_type {
    uint8_t channels;
    uint8_t format_type;
    uint8_t bitresolution;
    uint8_t sampfreq_num;
    uint8_t set_attributes_flag;
    uint32_t sampfreq[4];
};

/**
 * bSourceID in feature_unit = input_terminal_id
 * bSourceID in output_terminal = feature_unit_id
 * terminal_link_id = input_terminal_id or output_terminal_id (if input_terminal_type or output_terminal_type is 0x0101)
 *
 *
*/
struct usbh_audio_module {
    const char *name;
    uint8_t data_intf;
    uint8_t input_terminal_id;
    uint16_t input_terminal_type;
    uint16_t input_channel_config;
    uint8_t output_terminal_id;
    uint16_t output_terminal_type;
    uint8_t feature_unit_id;
    uint8_t feature_unit_controlsize;
    uint8_t feature_unit_controls[8];
    uint8_t terminal_link_id;
    struct usbh_audio_format_type altsetting[CONFIG_USBHOST_MAX_INTF_ALTSETTINGS];
};

struct usbh_audio {
    struct usbh_hubport *hport;

    uint8_t ctrl_intf; /* interface number */
    uint8_t minor;
    usbh_pipe_t isoin;  /* ISO IN endpoint */
    usbh_pipe_t isoout; /* ISO OUT endpoint */
    uint16_t isoin_mps;
    uint16_t isoout_mps;
    bool is_opened;
    uint16_t bcdADC;
    uint8_t bInCollection;
    uint8_t num_of_intf_altsettings;
    struct usbh_audio_module module[2];
    uint8_t module_num;
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

int usbh_audio_alloc_pipe(struct usbh_audio *audio_class, const char *name, uint32_t samp_freq);
#if CONFIG_BK_USB_CHERRYUSB_V1_6
/* v1.6 host audio adds an explicit bit-resolution selector. */
int usbh_audio_open(struct usbh_audio *audio_class, const char *name, uint32_t samp_freq, uint8_t bitresolution);
#else
int usbh_audio_open(struct usbh_audio *audio_class, const char *name, uint32_t samp_freq);
#endif
int usbh_audio_close(struct usbh_audio *audio_class, const char *name);
int usbh_audio_set_volume(struct usbh_audio *audio_class, const char *name, uint8_t ch, uint8_t volume);
int usbh_audio_set_mute(struct usbh_audio *audio_class, const char *name, uint8_t ch, bool mute);

void usbh_audio_run(struct usbh_audio *audio_class);
void usbh_audio_stop(struct usbh_audio *audio_class);

void bk_usbh_audio_sw_init(struct usbh_hubport *hport, uint8_t interface_num, uint8_t interface_sub_class);
void bk_usbh_audio_sw_deinit(struct usbh_hubport *hport, uint8_t interface_num, uint8_t interface_sub_class);

void bk_usbh_audio_unregister_dev(void);

void usbh_uac_class_register();

#ifdef __cplusplus
}
#endif

#endif /* USBH_AUDIO_H */
