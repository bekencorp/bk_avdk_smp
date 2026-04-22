// Copyright 2023-2024 Beken
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


#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "uvc_urb_list.h"
#include <common/avdk_pixel_types.h>
#if CONFIG_SOC_SMP
#include "spinlock.h"
#endif

#define TAG "uvc_urb"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGV(TAG, ##__VA_ARGS__)

uvc_urb_list_t g_uvc_list = {0};

#if CONFIG_SOC_SMP
static SPINLOCK_SECTION volatile spinlock_t s_uvc_urb_spin_lock = SPIN_LOCK_INIT;
#endif

static inline uint32_t uvc_urb_list_enter_critical(void)
{
    uint32_t flags = rtos_disable_int();

#if CONFIG_SOC_SMP
    spin_lock(&s_uvc_urb_spin_lock);
#endif

    return flags;
}

static inline void uvc_urb_list_exit_critical(uint32_t flags)
{
#if CONFIG_SOC_SMP
    spin_unlock(&s_uvc_urb_spin_lock);
#endif

    rtos_enable_int(flags);
}

bk_err_t uvc_camera_urb_list_init(void)
{
    int ret = BK_OK;
    uint32_t flags;
    uvc_urb_list_t *mem_list = &g_uvc_list;
    uint32_t offset0 = 0, offset1 = 0;

    flags = uvc_urb_list_enter_critical();
    if (mem_list->enable)
    {
        uvc_urb_list_exit_critical(flags);
        LOGD("%s, urb list already init\r\n", __func__);
        return ret;
    }
    mem_list->deiniting = false;
    uvc_urb_list_exit_critical(flags);

    INIT_LIST_HEAD(&mem_list->free);
    INIT_LIST_HEAD(&mem_list->ready);

    rtos_init_mutex(&mem_list->lock);

    rtos_init_semaphore(&mem_list->sem, 1);

    mem_list->count = UVC_URB_MAX_NUM;
    mem_list->size = UVC_URB_MAX_NUM * UVC_NUM_PACKET_PER_URB * UVC_MAX_PACKET_SIZE;

#ifdef CONFIG_UVC_USE_PSRAM_ALLOC
    mem_list->buffer = (uint8_t *)psram_malloc(mem_list->size);
#else
    mem_list->buffer = (uint8_t *)os_malloc(mem_list->size);
#endif

    LOGD("%s, mem_list->buffer:%p, size:%d\r\n", __func__, mem_list->buffer, mem_list->size);

    if (mem_list->buffer == NULL)
    {
        LOGE("%s, malloc buffer error\r\n", __func__);
        BK_ASSERT(0);
        ret = BK_FAIL;
    }

    for (uint8_t i = 0; i < mem_list->count; i++)
    {
#ifdef CONFIG_UVC_USE_PSRAM_ALLOC
        uvc_urb_node_t *node = (uvc_urb_node_t *)psram_malloc(sizeof(uvc_urb_node_t)
                               + sizeof(struct usbh_iso_frame_packet) * UVC_NUM_PACKET_PER_URB);
#else
        uvc_urb_node_t *node = (uvc_urb_node_t *)os_malloc(sizeof(uvc_urb_node_t)
                               + sizeof(struct usbh_iso_frame_packet) * UVC_NUM_PACKET_PER_URB);
#endif
        if (node == NULL)
        {
            LOGE("%s malloc node failed\n", __func__);
            BK_ASSERT(0);
            return BK_FAIL;
        }

        os_memset(node, 0, sizeof(uvc_urb_node_t)
                  + sizeof(struct usbh_iso_frame_packet) * UVC_NUM_PACKET_PER_URB);

        node->urb.num_of_iso_packets = UVC_NUM_PACKET_PER_URB;
        node->urb.transfer_buffer = mem_list->buffer + offset0;
        node->urb.transfer_buffer_length = UVC_MAX_PACKET_SIZE * node->urb.num_of_iso_packets;
        offset0 += node->urb.transfer_buffer_length;

        LOGD("node(%d): transfer_buffer:%p, transfer_buffer_length:%d\r\n",
             i,
             node->urb.transfer_buffer,
             node->urb.transfer_buffer_length);

        os_memset(node->urb.iso_packet, 0, sizeof(struct usbh_iso_frame_packet) * UVC_NUM_PACKET_PER_URB);

        offset1 = 0;

        for (uint8_t j = 0; j < node->urb.num_of_iso_packets; j++)
        {
            node->urb.iso_packet[j].transfer_buffer = node->urb.transfer_buffer + offset1;
            node->urb.iso_packet[j].transfer_buffer_length = UVC_MAX_PACKET_SIZE;
            node->urb.iso_packet[j].actual_length = 0;
            node->urb.iso_packet[j].errorcode = 0;
            offset1 += UVC_MAX_PACKET_SIZE;

            LOGD("iso_packet(%d): transfer_buffer:%p, transfer_buffer_length:%d\r\n",
                 j, node->urb.iso_packet[j].transfer_buffer,
                 node->urb.iso_packet[j].transfer_buffer_length);
        }

        LOGD("%s, %d, %p\r\n", __func__, __LINE__, &node->urb);

        list_add_tail(&node->list, &mem_list->free);
    }

    flags = uvc_urb_list_enter_critical();
    mem_list->enable = true;
    mem_list->deiniting = false;
    uvc_urb_list_exit_critical(flags);

    return ret;
}

bk_err_t uvc_camera_urb_list_deinit(void)
{
    int ret = BK_OK;
    uvc_urb_list_t *mem_list = NULL;
    uint8_t *buffer = NULL;
    uvc_urb_node_t *tmp = NULL;
    LIST_HEADER_T free_list;
    LIST_HEADER_T ready_list;
    LIST_HEADER_T *pos, *n;
    uint32_t flags;

    mem_list = &g_uvc_list;

    INIT_LIST_HEAD(&free_list);
    INIT_LIST_HEAD(&ready_list);

    flags = uvc_urb_list_enter_critical();
    if (mem_list->enable == false)
    {
        uvc_urb_list_exit_critical(flags);
        LOGE("%s already deinit\n", __func__);
        return ret;
    }

    mem_list->deiniting = true;
    mem_list->enable = false;
    list_splice_init(&mem_list->free, &free_list);
    list_splice_init(&mem_list->ready, &ready_list);
    buffer = mem_list->buffer;
    mem_list->buffer = NULL;
    uvc_urb_list_exit_critical(flags);

    rtos_set_semaphore(&mem_list->sem);

    list_for_each_safe(pos, n, &free_list)
    {
        tmp = list_entry(pos, uvc_urb_node_t, list);
        LOGD("free list: %p\n", tmp);
        if (tmp != NULL)
        {
            list_del(pos);
            os_free(tmp);
            tmp = NULL;
        }
    }

    list_for_each_safe(pos, n, &ready_list)
    {
        tmp = list_entry(pos, uvc_urb_node_t, list);
        LOGD("ready list: %p\n", tmp);
        if (tmp != NULL)
        {
            list_del(pos);
            os_free(tmp);
            tmp = NULL;
        }
    }

    os_free(buffer);

    rtos_deinit_semaphore(&mem_list->sem);
    rtos_deinit_mutex(&mem_list->lock);

    LOGI("uvc urb list deinit finish\n");

    return ret;
}

void uvc_camera_urb_list_clear(void)
{
    uvc_urb_list_t *mem_list = NULL;
    uvc_urb_node_t *tmp = NULL;
    LIST_HEADER_T *pos, *n;
    uint32_t flags;

    mem_list = &g_uvc_list;

    flags = uvc_urb_list_enter_critical();
    if (mem_list->enable == false || mem_list->deiniting)
    {
        uvc_urb_list_exit_critical(flags);
        LOGE("%s already deinit\n", __func__);
        return;
    }

    list_for_each_safe(pos, n, &mem_list->ready)
    {
        tmp = list_entry(pos, uvc_urb_node_t, list);
        if (tmp != NULL)
        {
            list_move_tail(&tmp->list, &mem_list->free);
        }
    }

    uvc_urb_list_exit_critical(flags);
}

struct usbh_urb *uvc_camera_urb_malloc(void)
{
    uvc_urb_list_t *mem_list = NULL;
    uvc_urb_node_t *tmp = NULL, *node = NULL;
    LIST_HEADER_T *pos, *n;
    uint32_t flags;

    mem_list = &g_uvc_list;

    flags = uvc_urb_list_enter_critical();
    if (mem_list->enable == false || mem_list->deiniting)
    {
        uvc_urb_list_exit_critical(flags);
        LOGE("%s already deinit\n", __func__);
        return NULL;
    }

    list_for_each_safe(pos, n, &mem_list->free)
    {
        tmp = list_entry(pos, uvc_urb_node_t, list);
        if (tmp != NULL)
        {
            node = tmp;
            list_del(pos);
            break;
        }
    }
    uvc_urb_list_exit_critical(flags);

    if (node == NULL)
    {
        LOGE("%s failed\n", __func__);
        return NULL;
    }

    node->urb.actual_length = 0;

    for (uint8_t j = 0; j < node->urb.num_of_iso_packets; j++)
    {
        node->urb.iso_packet[j].actual_length = 0;
        node->urb.iso_packet[j].errorcode = 0;
    }

    LOGD("%s, node:%p, %p\r\n", __func__, node, &node->urb);
    return &node->urb;
}

void uvc_camera_urb_free(struct usbh_urb *urb)
{
    uvc_urb_list_t *mem_list = NULL;
    uvc_urb_node_t *node = list_entry(urb, uvc_urb_node_t, urb);
    uint32_t flags;

    mem_list = &g_uvc_list;

    flags = uvc_urb_list_enter_critical();
    if (mem_list->enable == false || mem_list->deiniting)
    {
        uvc_urb_list_exit_critical(flags);
        LOGE("%s already deinit\n", __func__);
        return;
    }

    urb->pipe = NULL;
    list_add_tail(&node->list, &mem_list->free);
    uvc_urb_list_exit_critical(flags);
}

void uvc_camera_urb_push(struct usbh_urb *urb)
{
    uvc_urb_list_t *mem_list = NULL;
    uvc_urb_node_t *node = list_entry(urb, uvc_urb_node_t, urb);
    uint32_t flags;

    mem_list = &g_uvc_list;

    flags = uvc_urb_list_enter_critical();
    if (mem_list->enable == false || mem_list->deiniting)
    {
        uvc_urb_list_exit_critical(flags);
        LOGE("%s already deinit\n", __func__);
        return;
    }

    list_add_tail(&node->list, &mem_list->ready);
    uvc_urb_list_exit_critical(flags);

    rtos_set_semaphore(&mem_list->sem);
}

struct usbh_urb *uvc_camera_urb_pop(void)
{
    uvc_urb_list_t *mem_list = NULL;
    uvc_urb_node_t *tmp = NULL, *node = NULL;
    LIST_HEADER_T *pos, *n;
    uint32_t flags;

    mem_list = &g_uvc_list;

    flags = uvc_urb_list_enter_critical();
    if (mem_list->enable == false || mem_list->deiniting)
    {
        uvc_urb_list_exit_critical(flags);
        LOGE("%s already deinit\n", __func__);
        return NULL;
    }

    list_for_each_safe(pos, n, &mem_list->ready)
    {
        tmp = list_entry(pos, uvc_urb_node_t, list);
        if (tmp != NULL)
        {
            node = tmp;
            list_del(pos);
            break;
        }
    }
    uvc_urb_list_exit_critical(flags);

    if (node == NULL)
    {
        flags = uvc_urb_list_enter_critical();
        if (mem_list->enable == false || mem_list->deiniting)
        {
            uvc_urb_list_exit_critical(flags);
            return NULL;
        }
        uvc_urb_list_exit_critical(flags);

        if (rtos_get_semaphore(&mem_list->sem, 100) != BK_OK)
        {
            LOGD("%s, get node timeout, do not urb push!\r\n", __func__);
        }
        return NULL;
    }

    return &node->urb;
}
