#pragma once

#include <common/bk_err.h>
#include <common/bk_include.h>

#include <os/os.h>
#include <os/mem.h>
#include <stdio.h>

#include <components/usbh_hub_multiple_classes_api.h>
#include <components/bk_uvc_camera_types.h>

#include "FreeRTOS.h"
#include "event_groups.h"

#ifdef CONFIG_UVC_FRAME_SIZE
#define UVC_FRAME_SIZE (CONFIG_UVC_FRAME_SIZE)
#else
#define UVC_FRAME_SIZE (102400 * 2)
#endif

#define INDEX_MASK(bit)   (1U << bit)
#define INDEX_UNMASK(bit) (~(1U << bit))

#define UVC_STREAM_TASK_ENABLE_BIT   INDEX_MASK(0)
#define UVC_STREAM_TASK_DISABLE_BIT  INDEX_MASK(1)
#define UVC_PROCESS_TASK_ENABLE_BIT  INDEX_MASK(2)
#define UVC_PROCESS_TASK_DISABLE_BIT INDEX_MASK(3)
#define UVC_PROCESS_TASK_START_BIT   INDEX_MASK(4)
#define UVC_STREAM_START_BIT         INDEX_MASK(5)
#define UVC_STREAM_STOP_BIT          INDEX_MASK(6)
#define UVC_STREAM_SUSPEND_BIT       INDEX_MASK(7)
#define UVC_STREAM_RESUME_BIT        INDEX_MASK(8)
#define UVC_PORT_IDLE_BIT(port)      INDEX_MASK((8 + (port)))



#define UVC_TIME_INTERVAL       (4)

//#define UVC_DEBUG_TIME

#ifdef UVC_DEBUG_TIME
#define UVC_INIT_START()                GPIO_UP(31);
#define UVC_INIT_END()                  GPIO_DOWN(31);

#define UVC_PACKET_PUSH_START()         GPIO_UP(32);
#define UVC_PACKET_PUSH_END()           GPIO_DOWN(32);

#define UVC_PACKET_PROCESS_START()      GPIO_UP(33);
#define UVC_PACKET_PROCESS_END()        GPIO_DOWN(33);

#define UVC_PACKET_NEW_FRAME_BIT_START() GPIO_UP(34);
#define UVC_PACKET_NEW_FRAME_BIT_END()   GPIO_DOWN(34);

#define UVC_PACKET_EOF_BIT_START()      GPIO_UP(35);
#define UVC_PACKET_EOF_BIT_END()        GPIO_DOWN(35);

#define UVC_PACKET_ERROR_START()        GPIO_UP(36);
#define UVC_PACKET_ERROR_END()          GPIO_DOWN(36);

#define UVC_PACKET_EMPTY_START()        GPIO_UP(37);
#define UVC_PACKET_EMPTY_END()          GPIO_DOWN(37);

#define UVC_PACKET_DMA_START()          GPIO_UP(38);
#define UVC_PACKET_DMA_END()            GPIO_DOWN(38);

#define UVC_PACKET_EOF_START()          GPIO_UP(39);
#define UVC_PACKET_EOF_END()            GPIO_DOWN(39);
#else
#define UVC_INIT_START()
#define UVC_INIT_END()

#define UVC_PACKET_PUSH_START()
#define UVC_PACKET_PUSH_END()

#define UVC_PACKET_PROCESS_START()
#define UVC_PACKET_PROCESS_END()

#define UVC_PACKET_NEW_FRAME_BIT_START()
#define UVC_PACKET_NEW_FRAME_BIT_END()

#define UVC_PACKET_EOF_BIT_START()
#define UVC_PACKET_EOF_BIT_END()

#define UVC_PACKET_ERROR_START()
#define UVC_PACKET_ERROR_END()

#define UVC_PACKET_EMPTY_START()
#define UVC_PACKET_EMPTY_END()

#define UVC_PACKET_DMA_START()
#define UVC_PACKET_DMA_END()

#define UVC_PACKET_EOF_START()
#define UVC_PACKET_EOF_END()
#endif


typedef enum
{
    UVC_STREAM_CLOSED_STATE = 0,
    UVC_STREAM_CLOSING_STATE,
    UVC_STREAM_CONFIGING_STATE,
    UVC_STREAM_STREAMING_STATE,
    UVC_STREAM_CONNECTED_STATE,
    UVC_STREAM_DISCONNECTED_STATE,
} uvc_stream_state_t;

typedef enum
{
    UVC_CONNECT_IND = 0,
    UVC_DISCONNECT_IND,
    UVC_STREAM_START_IND,
    UVC_STREAM_STOP_IND,
    UVC_STREAM_SUSPEND_IND,
    UVC_STREAM_RESUME_IND,
    UVC_DATA_REQUEST_IND,
    UVC_EOF_IND,
    UVC_EXIT_IND,
    UVC_UNKNOW_IND,
} uvc_event_t;

typedef struct
{
    uvc_event_t event;
    uint32_t param;
} uvc_msg_t;

typedef struct
{
    uint8_t *frame;
    uint8_t port;
} uvc_eof_param_t;

typedef struct
{
    beken_semaphore_t sem;
    uint8_t port;
    volatile uint8_t stop_requested;
    volatile uint8_t processing;
    volatile uint32_t pending_urb_num;
    bk_cam_uvc_config_t *info;
    uint8_t skip_frames;           /**< AE warmup: configured skip count, set via BK_UVC_IOCTL_SET_SKIP_FRAMES */
    uint8_t skip_frames_remaining; /**< AE warmup: skip first N complete frames before upper layer */
    uvc_stream_state_t stream_state;
    struct usbh_urb *urb;
    frame_buffer_t *frame;
    bk_usb_hub_port_info *port_info;
} uvc_param_t;

typedef struct
{
    uint8_t transfer_bulk[UVC_PORT_MAX];// transfer ways, 1:for bulk, 0:for iso
    uint8_t packet_error[UVC_PORT_MAX];
    uint8_t head_bit0[UVC_PORT_MAX];
    uint8_t dma[UVC_PORT_MAX];
    uint16_t max_packet_size[UVC_PORT_MAX]; // transfer max packet size
    uint32_t frame_id[UVC_PORT_MAX];

#ifdef CONFIG_UVC_DEBUG_TIMER_ENABLE
    beken_timer_t timer;
    uint32_t later_id[UVC_PORT_MAX];
    uint32_t curr_length[UVC_PORT_MAX];
    uint32_t all_packet_num;
    uint32_t packet_err_num;
#endif // CONFIG_UVC_DEBUG_TIMER_ENABLE
} uvc_pro_config_t;

typedef struct
{
    uint8_t stream_enable;
    uint8_t pro_enable;
    beken_event_t handle;
    beken_thread_t stream_thread;
    beken_queue_t stream_queue;
    beken_thread_t pro_thread;
    beken_mutex_t lock;
    uvc_param_t camera[UVC_PORT_MAX];
    uvc_pro_config_t *pro_config; // uvc process config info
    void (*packet_cb)(struct usbh_urb *urb);
    const bk_uvc_callback_t *callback;
} uvc_stream_handle_t;

typedef struct {
    bk_cam_uvc_config_t config;
    const bk_uvc_callback_t *callback;
    uvc_stream_handle_t *stream_handle;
    bk_uvc_ctlr_t ops;
} private_uvc_ctlr_t;


avdk_err_t bk_uvc_camera_stream_init(uvc_stream_handle_t **handle, const bk_uvc_callback_t *callback);
avdk_err_t bk_uvc_camera_stream_deinit(uvc_stream_handle_t *handle);
avdk_err_t bk_uvc_camera_stream_start(uvc_stream_handle_t *handle, bk_cam_uvc_config_t *config);
avdk_err_t bk_uvc_camera_stream_stop(uvc_stream_handle_t *handle, uint8_t port);
avdk_err_t bk_uvc_camera_stream_suspend(uvc_stream_handle_t *handle, uint8_t port);
avdk_err_t bk_uvc_camera_stream_resume(uvc_stream_handle_t *handle, uint8_t port);
avdk_err_t bk_uvc_camera_stream_ioctl(uvc_stream_handle_t *handle, bk_uvc_ioctl_cmd_t event, void *arg);