// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <common/bk_err.h>
#include <driver/int.h>
#include "bk_list.h"
#include <components/usb_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UVC_MAX_PACKET_SIZE (1024)
#ifdef CONFIG_UVC_URB_NUM
#define UVC_URB_MAX_NUM (CONFIG_UVC_URB_NUM)
#else
#define UVC_URB_MAX_NUM (8)
#endif

#ifdef CONFIG_UVC_NUM_PACKET_PER_URB
#define UVC_NUM_PACKET_PER_URB (CONFIG_UVC_NUM_PACKET_PER_URB)
#else
#define UVC_NUM_PACKET_PER_URB (8)
#endif

extern uint32_t  platform_is_in_interrupt_context(void);

typedef struct
{
    LIST_HEADER_T free;
    LIST_HEADER_T ready;
    beken_mutex_t lock;
    beken_semaphore_t sem;
    uint8_t  enable;
    uint8_t  count;
    uint8_t *buffer;
    uint32_t size;
} uvc_urb_list_t;

typedef struct
{
    LIST_HEADER_T list;
    struct usbh_urb urb;
} uvc_urb_node_t;

bk_err_t uvc_camera_urb_list_init(void);
bk_err_t uvc_camera_urb_list_deinit(void);
void uvc_camera_urb_list_clear(void);

struct usbh_urb *uvc_camera_urb_malloc(void);
void uvc_camera_urb_free(struct usbh_urb *urb);
void uvc_camera_urb_push(struct usbh_urb *urb);
struct usbh_urb *uvc_camera_urb_pop(void);


#if (CONFIG_STANDARD_DUALSTREAM)

bk_err_t uvc_camera_h26x_urb_list_init(void);
bk_err_t uvc_camera_h26x_urb_list_deinit(void);
void uvc_camera_h26x_urb_list_clear(void);

struct usbh_urb *uvc_camera_h26x_urb_malloc(void);
void uvc_camera_h26x_urb_free(struct usbh_urb *urb);
void uvc_camera_h26x_urb_push(struct usbh_urb *urb);
struct usbh_urb *uvc_camera_h26x_urb_pop(void);

#endif

#ifdef __cplusplus
}
#endif
