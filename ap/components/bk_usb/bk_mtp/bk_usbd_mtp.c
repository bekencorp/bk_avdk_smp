#include "bk_usb_mtp.h"
#include <os/mem.h>
#include <os/os.h>
#include <soc/soc.h>
#include <modules/pm.h>
#include "usbd_core.h"
#include "usb_osal.h"

/* Device-mode USB analog PHY / clock / OTG (B-device) bring-up.
 *
 * The v1.6 device path never powers the MUSB analog PHY: msc_storage_init() and
 * usb_mtp_init() go straight to usbd_initialize(), and the v1.6 glue's
 * usb_dc_low_level_init() (unlike the legacy one) dropped the analog/clock/OTG
 * power-up. Result: DEVCTL/POWER read 0 and a PC never sees the gadget connect
 * (no RESET/CONFIGURED). The host path avoids this because bk_usb_open(HOST)
 * runs this exact sequence. Mirror it here for the device, and -- importantly --
 * do it from the CLI-triggered (post-boot, system-settled) MTP bring-up rather
 * than the 92 ms boot init, so the analog SPI/clock pokes don't race the running
 * RISC-V CP (which earlier produced an ~8 s heartbeat-timeout coredump).
 *
 * bk_analog_layer_usb_sys_related_ops() is defined in
 * CherryUSB/driver/usb_driver.c. usb_mode:
 * 0 = USB_HOST_MODE, 1 = USB_DEVICE_MODE. */
extern void bk_analog_layer_usb_sys_related_ops(uint32_t usb_mode, bool ops);
#define MTP_USB_DEVICE_MODE 1

#if CONFIG_VFS
#include "bk_posix.h"
#include "bk_filesystem.h"
#include "bk_fdtable.h"
#include "bk_file_utils.h"
#include "conv_utf8_pub.h"
#endif

/* ============================================================
 * CherryUSB v0.7 -> v1.6 device-API compatibility shims
 *
 * The glass MTP engine was written against the old (busid-less) CherryUSB
 * device API. v1.6 prepends a busid to the endpoint/transfer calls. Remap the
 * call sites with function-like macros; the parenthesized real name suppresses
 * recursive macro expansion so the actual v1.6 function is invoked.
 * ============================================================ */
#define MTP_BUSID 0
#define usbd_ep_start_write(ep, buf, len) (usbd_ep_start_write)(MTP_BUSID, (ep), (buf), (len))
#define usbd_ep_start_read(ep, buf, len)  (usbd_ep_start_read)(MTP_BUSID, (ep), (buf), (len))
#define usbd_ep_set_stall(ep)             (usbd_ep_set_stall)(MTP_BUSID, (ep))

/* glass power-manager hooks are optional; stub to no-ops here. */
#define malloc_pm_lock()  (1)
#define pm_lock(x)        do { (void)(x); } while (0)
#define pm_unlock(x)      do { (void)(x); } while (0)

/* glass_fs_* are only used by the (optional) FORMAT_STORE handler; stub to OK. */
#define glass_fs_mount(path)   (0)
#define glass_fs_umount(path)  (0)
#define glass_fs_format(path)  (0)

/* MTP reports a battery level for the BATTERY_LEVEL device property. glass
 * sourced it from its battery_manage component; provide a weak default (full)
 * so projects without a battery still link. A real get_battery_level() in the
 * application overrides this. */
__attribute__((weak)) uint8_t get_battery_level(void)
{
    return 100;
}

#define USBD_VID           0x25A7
#define USBD_PID           0x0100
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

#ifndef CONFIG_USBD_MTP_PRODUCT_NAME
#define CONFIG_USBD_MTP_PRODUCT_NAME "BekenMTP"
#endif

#ifndef CONFIG_USBD_MTP_DEVICE_TYPE
#define CONFIG_USBD_MTP_DEVICE_TYPE "1"
#endif

#define MTP_DEVICE_TYPE_DEFAULT 1U

static uint32_t mtp_get_perceived_device_type(void)
{
    const char *type = CONFIG_USBD_MTP_DEVICE_TYPE;

    if (type && type[0] >= '0' && type[0] <= '6' && type[1] == '\0') {
        return (uint32_t)(type[0] - '0');
    }

    return MTP_DEVICE_TYPE_DEFAULT;
}

#define MTP_IN_EP_IDX   0
#define MTP_OUT_EP_IDX  1
#define MTP_INT_EP_IDX  2

#define MTP_IN_EP  0x81
#define MTP_OUT_EP 0x01
#define MTP_INT_EP 0x82

#define MTP_BUFFER_SIZE 512
#define MTP_COPY_BUFFER_SIZE (10*1024)

static uint32_t mtp_put_ascii_string(uint8_t *dst, const char *src)
{
    uint32_t chars = 0;

    while (src[chars] != '\0' && chars < 254) {
        chars++;
    }

    dst[0] = (uint8_t)(chars + 1);
    for (uint32_t i = 0; i < chars; i++) {
        dst[1 + i * 2] = (uint8_t)src[i];
        dst[2 + i * 2] = 0x00;
    }
    dst[1 + chars * 2] = 0x00;
    dst[2 + chars * 2] = 0x00;

    return 1 + (chars + 1) * 2;
}

static uint32_t mtp_string_size(const uint8_t *str)
{
    return 1 + ((uint32_t)str[0] * 2);
}

static uint32_t mtp_get_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static const uint8_t *mtp_skip_string(const uint8_t *p)
{
    return p + mtp_string_size(p);
}

static const uint8_t *mtp_skip_u16_array(const uint8_t *p)
{
    return p + 4 + mtp_get_le32(p) * 2;
}

#ifdef CONFIG_USB_HS
#define MAX_PACKET_SIZE 512
#else
#define MAX_PACKET_SIZE 64
#endif

#define LEARR(x) (x&0xff),((x>>8)&0xff)

enum{
    MTP_STATE_IDLE,
    MTP_STATE_SEND_RSP,
    MTP_STATE_SEND_DATA,
    MTP_STATE_RECIVE_OBJ_DATA,
    MTP_STATE_SEND_EVENT,
    MTP_STATE_SEND_OBJ_DATA,
};

enum{
    MTP_THREAD_OP_NONE,
    MTP_THREAD_OP_GET_STORAGE_INFO,
    MTP_THREAD_OP_GET_OBJECT_HANDLES,
    MTP_THREAD_OP_GET_OBJECT_INFO,
    MTP_THREAD_OP_GET_OBJECT_NUM,
    MTP_THREAD_OP_SEND_OBJECT_INFO,
    MTP_THREAD_OP_SEND_OBJECT,
    MTP_THREAD_OP_MOVE_OBJECT,
    MTP_THREAD_OP_COPY_OBJECT,
    MTP_THREAD_OP_GET_OBJECT,
    MTP_THREAD_OP_GET_OBJECT_SIZE,
    MTP_THREAD_OP_GET_OBJECT_FILE_NAME,
    MTP_THREAD_OP_SET_OBJECT_PROP_VALUE,
    MTP_THREAD_OP_DELETE_OBJECT,
    MTP_THREAD_OP_GET_OBJECT_PROPLIST,
    MTP_THREAD_OP_FORMAT_STORAGE,
    MTP_THREAD_OP_RESET,
    MTP_THREAD_OP_CANCEL_REQUEST,
    MTP_THREAD_EXIT,
};

typedef struct
{
    uint32_t container_length;
    uint16_t container_type;
    uint16_t code;
    uint32_t transaction_id;
    union
    {
        uint32_t parameter[5];
        uint8_t payload[0];
    };
}mtp_packet_t;

typedef struct __PACKED
{
    uint16_t storage_type;
    uint16_t filesystem_type;
    uint16_t access_capability;
    uint64_t max_capacity;
    uint64_t free_space_in_bytes;
    uint32_t free_space_in_objects;
    uint8_t storage_description_str[0];
}mtp_storageinfo_dataset_t;

typedef struct __PACKED
{
    uint32_t storage_id;
    uint16_t object_format;
    uint16_t protection_status;
    uint32_t object_compressed_size;
    uint16_t thumb_format;
    uint32_t thumb_compressed_size;
    uint32_t thumb_pix_width;
    uint32_t thumb_pix_height;
    uint32_t image_pix_width;
    uint32_t image_pix_height;
    uint32_t image_bit_depth;
    uint32_t parent_object;
    uint16_t association_type;
    uint32_t association_deseription;
    uint32_t sequence_number;
    uint8_t str[0];
}mtp_objectinfo_dataset_t;

#if CONFIG_VFS

typedef struct _handle_map_item
{
    struct _handle_map_item *next;
    uint32_t handle;
    uint32_t parent_handle;
    char *str;
    uint8_t is_dir;
    uint8_t storage_id;
}handle_map_item_t;

typedef struct
{
    const char *mount_path;
    const uint8_t *name;
    const uint8_t *identifier;
}storage_info_t;

static const uint8_t sd_name[] = {
    0x08,'S',0x00,'D',0x00,' ',0x00,'C',0x00,'a',0x00,'r',0x00,'d',0x00,0x00,0x00
};

static const uint8_t sd_identifier[] = {
    0x03,'0',0x00,'1',0x00,0x00,0x00
};

static const storage_info_t storage_info[] = 
{
    {
        .mount_path = VFS_SD_0_PATITION_0,
        .name = sd_name,
        .identifier = sd_identifier,
    },
};

#define NUM_OF_STORAGE (sizeof(storage_info)/sizeof(storage_info_t))

static handle_map_item_t *path_list = NULL;
static handle_map_item_t *file_list = NULL;
static uint32_t next_handle;

static int open_fd;
static uint32_t last_sendinfo_handle;

static bool s_hidden_file_enable = false;
static const char **s_hidden_file_list = NULL;
static uint32_t s_hidden_file_list_count = 0;
static int _usb_mtp_hidden_file(const char *path);

static uint8_t check_handle_exist(uint32_t handle)
{
    handle_map_item_t *list = path_list;
    while(list)
    {
        if(handle == list->handle)
        {
            return 1;
        }
        list = list->next;
    }
    list = file_list;
    while(list)
    {
        if(handle == list->handle)
        {
            return 2;
        }
        list = list->next;
    }
    return 0;
}

static handle_map_item_t *get_list_item_by_handle(uint32_t handle)
{
    handle_map_item_t *list = path_list;
    while(list)
    {
        if(handle == list->handle)
        {
            return list;
        }
        list = list->next;
    }
    list = file_list;
    while(list)
    {
        if(handle == list->handle)
        {
            return list;
        }
        list = list->next;
    }
    return NULL;
}

static uint8_t remove_item_form_list_by_handle(handle_map_item_t **lst,uint32_t handle)
{
    handle_map_item_t *prev = NULL;
    handle_map_item_t *list = *lst;
    while(list)
    {
        if(list->handle == handle)
        {
            break;
        }
        prev = list;
        list = list->next;
    }
    if(list)
    {
        if(prev == NULL)
        {
            *lst = list->next;
        }
        else
        {
            prev->next = list->next;
        }
        psram_free(list->str);
        psram_free(list);
        return 1;
    }
    return 0;
}

static void remove_item_by_handle(uint32_t handle)
{
    remove_item_form_list_by_handle(&path_list,handle);
    remove_item_form_list_by_handle(&file_list,handle);
}

static void remove_object_recursive_by_handle(uint32_t handle)
{
    handle_map_item_t *item = get_list_item_by_handle(handle);
    if(item->is_dir)
    {
        uint8_t removed;
        do{
            removed = 0;
            handle_map_item_t *prev = NULL;
            handle_map_item_t *list = file_list;
            while(list)
            {
                if(handle == list->parent_handle)
                {
                    if(prev == NULL)
                    {
                        file_list = list->next;
                    }
                    else
                    {
                        prev->next = list->next;
                    }
                    psram_free(list->str);
                    psram_free(list);
                    removed = 1;
                    break;
                }
                prev = list;
                list = list->next;
            }
        }while(removed);

        do{
            removed = 0;
            handle_map_item_t *list = path_list;
            while(list)
            {
                if(handle == list->parent_handle)
                {
                    break;
                }
                list = list->next;
            }
            if(list)
            {
                remove_object_recursive_by_handle(list->handle);
                removed = 1;
            }
        }while(removed);
        remove_item_form_list_by_handle(&path_list,item->handle);
    }
    else
    {
        remove_item_form_list_by_handle(&file_list,handle);
    }
}

/* Recursively delete a directory (and its contents) from the filesystem.
 * f_unlink/rmdir only removes an EMPTY dir, so the children must be removed
 * first; otherwise deleting a non-empty folder over MTP silently fails. Child
 * paths are heap-allocated (not stack) to stay safe on the small worker stack.
 * Returns 0 on success. */
static int mtp_rmdir_recursive(const char *path)
{
    DIR *dir = opendir(path);
    if(dir)
    {
        struct dirent *de;
        while((de = readdir(dir)) != NULL)
        {
            if(strcmp(de->d_name,".") == 0 || strcmp(de->d_name,"..") == 0)
                continue;
            char *child = psram_malloc(strlen(path) + strlen(de->d_name) + 2);
            if(child == NULL)
                continue;
            sprintf(child,"%s/%s",path,de->d_name);
            if(de->d_type & DT_DIR)
                mtp_rmdir_recursive(child);
            else
                unlink(child);
            psram_free(child);
        }
        closedir(dir);
    }
    return unlink(path); /* now empty -> f_unlink removes the directory itself */
}

static uint32_t map_add_item(uint32_t parent_handle,const char *path,const char* name,uint8_t is_dir,uint8_t storage_id)
{
    if(path == NULL || name == NULL) return 0;
    if(is_dir)
    {
        char *fullpath = psram_malloc(strlen(path) + strlen(name) + 2);
        if(fullpath == NULL)
        {
            USB_LOG_ERR("mtp malloc fail!\r\n");
            return 0;
        }
        sprintf(fullpath,"%s/%s",path,name);

        handle_map_item_t *list = path_list;
        while(list)
        {
            if(parent_handle == list->parent_handle && storage_id == list->storage_id && !strcmp(list->str,fullpath))
            {
                psram_free(fullpath);
                return list->handle;
            }
            list = list->next;
        }
        handle_map_item_t *item = psram_malloc(sizeof(handle_map_item_t));
        if(item == NULL)
        {
            USB_LOG_ERR("mtp malloc fail!\r\n");
            psram_free(fullpath);
            return 0;
        }
        item->is_dir = is_dir;
        item->storage_id = storage_id;
        item->parent_handle = parent_handle;
        item->handle = next_handle++;
        item->str = fullpath;
        item->next = path_list;
        path_list = item;
        return item->handle;
    }
    else
    {
        handle_map_item_t *list = file_list;
        while(list)
        {
            if(parent_handle == list->parent_handle && storage_id == list->storage_id && !strcmp(list->str,name))
            {
                return list->handle;
            }
            list = list->next;
        }
        handle_map_item_t *item = psram_malloc(sizeof(handle_map_item_t));
        if(item == NULL)
        {
            USB_LOG_ERR("mtp malloc fail!\r\n");
            return 0;
        }
        char *filename = psram_malloc(strlen(name) + 1);
        if(filename == NULL)
        {
            USB_LOG_ERR("mtp malloc fail!\r\n");
            psram_free(item);
            return 0;
        }
        strcpy(filename,name);
        item->is_dir = is_dir;
        item->storage_id = storage_id;
        item->parent_handle = parent_handle;
        item->handle = next_handle++;
        item->str = filename;
        item->next = file_list;
        file_list = item;
        return item->handle;
    }
}

static void clear_all_list_by_storage_id(uint16_t id)
{
    handle_map_item_t *prev = NULL;
    handle_map_item_t *list = path_list;
    while(list)
    {
        if(list->storage_id == id)
        {
            handle_map_item_t *temp = list;
            if(prev == NULL)
            {
                path_list = list->next;
            }
            else
            {
                prev->next = list->next;
            }
            list = list->next;
            psram_free(temp->str);
            psram_free(temp);
            continue;
        }
        prev = list;
        list = list->next;
    }
    prev = NULL;
    list = file_list;
    while(list)
    {
        if(list->storage_id == id)
        {
            handle_map_item_t *temp = list;
            if(prev == NULL)
            {
                file_list = list->next;
            }
            else
            {
                prev->next = list->next;
            }
            list = list->next;
            psram_free(temp->str);
            psram_free(temp);
            continue;
        }
        prev = list;
        list = list->next;
    }
}

static void clear_all_list(void)
{
    handle_map_item_t *list = path_list;
    while(list)
    {
        handle_map_item_t *temp = list;
        list = list->next;
        psram_free(temp->str);
        psram_free(temp);
    }
    path_list = NULL;
    list = file_list;
    while(list)
    {
        handle_map_item_t *temp = list;
        list = list->next;
        psram_free(temp->str);
        psram_free(temp);
    }
    file_list = NULL;
}

static char *get_full_path_by_item(handle_map_item_t *item)
{
    char *fullpath = NULL;
    if(item->parent_handle)
    {
        handle_map_item_t *path = get_list_item_by_handle(item->parent_handle);
        if(path == NULL || !path->is_dir) return NULL;
        fullpath = psram_malloc(strlen(path->str) + strlen(item->str) + 2);
        if(fullpath == NULL) return NULL;
        sprintf(fullpath,"%s/%s",path->str,item->str);
    }
    else
    {
        int idx = item->storage_id-1;
        fullpath = psram_malloc(strlen(storage_info[idx].mount_path) + strlen(item->str) + 2);
        if(fullpath == NULL) return NULL;
        sprintf(fullpath,"%s/%s",storage_info[idx].mount_path,item->str);
    }
    return fullpath;
}

static const char *get_file_name_from_path(const char *path)
{
    int len = strlen(path);
    int i;
    for(i = len-1; i>0 ;i--)
    {
        if(path[i] == '/')
            break;
    }
    return &path[i+1];
}

static int get_path_len(const char *path)
{
    int len = strlen(path);
    int i;
    for(i = len-1; i>0 ;i--)
    {
        if(path[i] == '/')
            break;
    }
    return i;
}

static int conver_ascii2utf16(const char *str,uint8_t *buf)
{
    int len = strlen(str);
    int idx = 1;
    for(int i = 0; i < len; i++)
    {
        if(str[i]&0x80)
        {
            buf[idx++] = str[i++];
            buf[idx++] = str[i];
        }
        else
        {
            buf[idx++] = str[i];
            buf[idx++] = 0x00;
        }
    }
    buf[idx++] = 0x00;
    buf[idx++] = 0x00;
    buf[0] = (idx-1)/2;
    return idx;
}

static void conver_utf162ascii(uint8_t *str,uint8_t *buf)
{
    buf[0] = 0;
    uint8_t num_char = str[0];
    str++;
    for(uint8_t i = 0; i < num_char; i++)
    {
        if((*(str+1))&0x80)
        {
            *buf++ = *str++;
            *buf++ = *str++;
        }
        else
        {
            *buf++ = *str++;
            str++;
        }
    }
}

static void Utf16LEToGb2312(uint8_t *str,uint8_t *buf)
{
    buf[0] = 0;
    uint8_t num_char = str[0];
    str++;
    uint8_t *utf8 = buf;

    for(uint8_t i = 0; i < num_char; i++)
    {
        uint16_t unicode = (str[1] << 8) | str[0];
        if(unicode < 0x80)
        {
            *utf8++ = (uint8_t)unicode;
        }
        else if(unicode < 0x800)
        {
            *utf8++ = (uint8_t)(0xc0|((unicode>>6)&0x1f));
            *utf8++ = (uint8_t)(0x80|(unicode&0x3f));
        }
        else
        {
            *utf8++ = (uint8_t)(0xe0|((unicode>>12)&0xf));
            *utf8++ = (uint8_t)(0x80|((unicode>>6)&0x3f));
            *utf8++ = (uint8_t)(0x80|(unicode&0x3f));
        }
        str += 2;
    }
    Utf8ToGb2312((char*)buf);
}

static int Gb2312ToUtf16LE(const char *gb2312,uint8_t *utf16le)
{
    int j = 1;
    uint8_t *utf8_temp = NULL;
    uint8_t *utf8_ptr = NULL;

    // First convert GB2312 to UTF-8
    utf8_temp = conv_utf8((unsigned char *)gb2312);
    if (!utf8_temp) {
        return 0;
    }

    utf8_ptr = utf8_temp;
    utf16le[0] = 0;
    // Convert UTF-8 to UTF16LE
    while (*utf8_ptr) {
        if ((*utf8_ptr & 0x80) == 0) {
            // ASCII character (0xxxxxxx)
            utf16le[j++] = *utf8_ptr;      // Little Endian, low byte first
            utf16le[j++] = 0x00;           // High byte
            utf8_ptr++;
            utf16le[0]++;
        } else if ((*utf8_ptr & 0xE0) == 0xC0) {
            // 2-byte UTF-8 (110xxxxx 10xxxxxx)
            uint16_t unicode = ((utf8_ptr[0] & 0x1F) << 6) | (utf8_ptr[1] & 0x3F);
            utf16le[j++] = (unicode & 0x00FF); // Little Endian, low byte first
            utf16le[j++] = (unicode & 0xFF00) >> 8; // High byte
            utf8_ptr += 2;
            utf16le[0]++;
        } else if ((*utf8_ptr & 0xF0) == 0xE0) {
            // 3-byte UTF-8 (1110xxxx 10xxxxxx 10xxxxxx)
            uint16_t unicode = ((utf8_ptr[0] & 0x0F) << 12) | 
                              ((utf8_ptr[1] & 0x3F) << 6) | 
                               (utf8_ptr[2] & 0x3F);
            utf16le[j++] = (unicode & 0x00FF); // Little Endian, low byte first
            utf16le[j++] = (unicode & 0xFF00) >> 8; // High byte
            utf8_ptr += 3;
            utf16le[0]++;
        } else {
            // For 4-byte UTF-8 or invalid UTF-8, we'll skip (can be enhanced later if needed)
            utf8_ptr++;
        }
    }

    // Add null terminator (two null bytes for UTF16LE)
    utf16le[j++] = 0x00;
    utf16le[j] = 0x00;
    utf16le[0]++;

    os_free(utf8_temp);
    return (utf16le[0]*2+1);
}

#endif

extern const unsigned int mtp_device_icon_len;
extern const unsigned char mtp_device_icon[];

static uint8_t s_mtp_init = 0;
static uint8_t usb_pm = 0 ;
static struct usbd_interface intf0;
static struct usbd_endpoint mtp_ep_data[3];
static uint8_t *ep_out_buffer = NULL;//[MAX_PACKET_SIZE];
static uint8_t *ep_in_buffer = NULL;//[MAX_PACKET_SIZE];

static uint8_t *mtp_buffer = NULL;//[MTP_BUFFER_SIZE];

static uint32_t current_session_id = 0;
static uint8_t mtp_state;
static uint8_t *current_sending_buf;
static uint32_t current_sending_total_size;
static uint32_t current_sending_idx;

static usb_osal_thread_t mtp_thread = NULL;
/* Join handshake: the worker gives this as its last act before self-deleting so
 * usb_mtp_deinit() can block until the worker has truly exited, then own the
 * sem/thread teardown itself. Previously the worker freed its wakeup object
 * asynchronously while a later start/stop reused the handle -> xQueueGenericSend
 * assert on a poisoned (0x55aa55aa) handle. */
static usb_osal_sem_t mtp_exit_sem = NULL;
static volatile uint8_t mtp_thread_op;
/* Authoritative exit request for the worker. Separate from mtp_thread_op because
 * a late USB SUSPEND/RESET event (mtp_notify_handler) can overwrite mtp_thread_op
 * after usb_mtp_deinit() asked the worker to quit; the worker must still break
 * out, otherwise the deinit join on mtp_exit_sem would block forever. */
static volatile uint8_t mtp_should_exit;
/* SMP-safe ISR->worker handoff. The old design signalled the worker with a
 * counting semaphore (max 1) + the shared mtp_thread_op var; under load the
 * cross-core give/wakeup intermittently failed and the worker stalled (host
 * spun on empty window). A FreeRTOS queue (depth 8) carries each request
 * atomically: usb_osal_mq_send uses xQueueSendToBackFromISR from the USB ISR
 * and the worker blocks on usb_osal_mq_recv, which is the platform's supported
 * ISR->task wakeup path. */
static usb_osal_mq_t mtp_op_q = NULL;
static uint32_t mtp_parameter_backup[5];
static uint32_t mtp_ep_recive_cnt;
static uint32_t mtp_ep_recive_total_size;

/* Set while the worker task is actively servicing an operation (including the
 * blocking SD writes of an in-flight SendObject). GetDeviceStatus reports
 * Device_Busy while this is set so a host that polls during a paused transfer
 * keeps waiting instead of treating the pause as a hang. */
static volatile uint8_t s_mtp_busy;

/* Retry a momentarily-busy SD card during SendObject file open/create/write.
 * The OUT endpoint stays un-armed for the whole retry window, so the USB host
 * transparently NAK-waits (progress bar just pauses) until the op lands.
 * Budget: MTP_SD_RETRY_MAX * MTP_SD_RETRY_MS = 1000 * 10ms = up to 10s of
 * patience per op -- for a file copy, data integrity beats latency, so we wait
 * out transient SD contention rather than truncating a file. Only if the card
 * is genuinely gone (budget fully exhausted) do we give up, and then LOUDLY
 * (delete the partial file + error response) so the host never silently leaves
 * a truncated/empty file behind. 10s is the worst-case worker block on a dead
 * card; tune down if a faster hard-fail is preferred. */
#define MTP_SD_RETRY_MS   10
#define MTP_SD_RETRY_MAX  1000

#if CONFIG_VFS
/* open() a file on the SD card, waiting out a transiently-busy card (see
 * MTP_SD_RETRY_*). Returns the fd (>=0) on success or the last negative error
 * once the retry budget is exhausted. The OUT endpoint stays un-armed during
 * the wait, so the USB host just NAK-pauses. */
static int mtp_sd_open_retry(const char *path, int flags)
{
    int fd;
    uint32_t tries = 0;
    while((fd = open(path, flags)) < 0)
    {
        if(++tries > MTP_SD_RETRY_MAX)
            break;
        usb_osal_msleep(MTP_SD_RETRY_MS);
    }
    return fd;
}
#endif

/* Post the current op to the worker queue. mtp_thread_op is already set by the
 * caller (kept for the live in-flight CANCEL check the worker does). */
static void mtp_wake_worker(void)
{
    if (mtp_op_q)
        (void)usb_osal_mq_send(mtp_op_q, (uintptr_t)mtp_thread_op);
}

const uint8_t mtp_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_1, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0200, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(0x27, 0x01, 0x01, USB_CONFIG_SELF_POWERED, USBD_MAX_POWER),
    USB_INTERFACE_DESCRIPTOR_INIT(0x00,0x00,0x03,USB_MTP_CLASS,USB_MTP_SUB_CLASS,USB_MTP_PROTOCOL,0x04),
#ifdef CONFIG_USB_HS
    USB_ENDPOINT_DESCRIPTOR_INIT(MTP_IN_EP,0x02,512,0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(MTP_OUT_EP,0x02,512,0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(MTP_INT_EP,0x03,28,0x18),
#else
    USB_ENDPOINT_DESCRIPTOR_INIT(MTP_IN_EP,0x02,64,0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(MTP_OUT_EP,0x02,64,0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(MTP_INT_EP,0x03,28,0x06),
#endif
    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(USBD_LANGID_STRING),
    ///////////////////////////////////////
    /// string1 descriptor
    ///////////////////////////////////////
    0x0C,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'B', 0x00,                  /* wcChar0 */
    'e', 0x00,                  /* wcChar1 */
    'k', 0x00,                  /* wcChar2 */
    'e', 0x00,                  /* wcChar3 */
    'n', 0x00,                  /* wcChar4 */
    ///////////////////////////////////////
    /// string2 descriptor
    ///////////////////////////////////////
    0x0C,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'G', 0x00,                  /* wcChar0 */
    'l', 0x00,                  /* wcChar1 */
    'a', 0x00,                  /* wcChar2 */
    's', 0x00,                  /* wcChar3 */
    's', 0x00,                  /* wcChar4 */
    ///////////////////////////////////////
    /// string3 descriptor
    ///////////////////////////////////////
    0x10,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '2', 0x00,                  /* wcChar0 */
    '0', 0x00,                  /* wcChar1 */
    '2', 0x00,                  /* wcChar2 */
    '5', 0x00,                  /* wcChar3 */
    '0', 0x00,                  /* wcChar4 */
    '0', 0x00,                  /* wcChar5 */
    '1', 0x00,                  /* wcChar6 */
    ///////////////////////////////////////
    /// string4 descriptor
    ///////////////////////////////////////
    0x08,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'M', 0x00,                  /* wcChar0 */
    'T', 0x00,                  /* wcChar1 */
    'P', 0x00,                  /* wcChar2 */
#ifdef CONFIG_USB_HS
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00,
#endif
    0x00
};

static const uint8_t mtp_bos_desc[] = {
    0x05,
    USB_DESCRIPTOR_TYPE_BINARY_OBJECT_STORE,
    0x0c & 0xff,
    (0x0c & 0xff00) >> 8,
    0x01,
    ///////////////////////////////////////
    /// USB 2.0 Extension Descriptor
    ///////////////////////////////////////
    0x07,
    USB_DESCRIPTOR_TYPE_DEVICE_CAPABILITY,
    0x02,
    0x06, 0x00, 0x00, 0x00,
};

static const uint8_t mtp_deviceinfo_dataset[] = {
    //standard version
    0x64,0x00,
    //MTP vendor extension id
    0x06,0x00,0x00,0x00,
    //MTP version
    0x64,0x00,
    //MTP extensions
    0x28,
    'm',0x00,'i',0x00,'c',0x00,'r',0x00,'o',0x00,'s',0x00,'o',0x00,'f',0x00,'t',0x00,'.',0x00,'c',0x00,'o',0x00,'m',0x00,':',0x00,' ',0x00,'1',0x00,'.',0x00,'0',0x00,';',0x00,' ',0x00,
    'b',0x00,'e',0x00,'k',0x00,'e',0x00,'n',0x00,'c',0x00,'r',0x00,'o',0x00,'p',0x00,'.',0x00,'c',0x00,'o',0x00,'m',0x00,':',0x00,' ',0x00,'1',0x00,'.',0x00,'0',0x00,';',0x00,0x00,0x00,
    //functional mode
    0x00,0x00,
    //operation supported
    0x18,0x00,0x00,0x00,
    LEARR(MTP_OP_GET_DEVICE_INFO),
    LEARR(MTP_OP_OPEN_SESSION),
    LEARR(MTP_OP_CLOSE_SESSION),
    LEARR(MTP_OP_GET_STORAGE_IDS),
    LEARR(MTP_OP_GET_STORAGE_INFO),
    LEARR(MTP_OP_GET_NUM_OBJECTS),
    LEARR(MTP_OP_GET_OBJECT_HANDLES),
    LEARR(MTP_OP_GET_OBJECT_INFO),
    LEARR(MTP_OP_GET_OBJECT),
    LEARR(MTP_OP_DELETE_OBJECT),
    LEARR(MTP_OP_SEND_OBJECT_INFO),
    LEARR(MTP_OP_SEND_OBJECT),
    LEARR(MTP_OP_FORMAT_STORE),
    LEARR(MTP_OP_GET_DEVICE_PROP_DESC),
    LEARR(MTP_OP_GET_DEVICE_PROP_VALUE),
    LEARR(MTP_OP_MOVE_OBJECT),
    LEARR(MTP_OP_COPY_OBJECT),
    LEARR(MTP_OP_GET_PARTIAL_OBJECT),
    LEARR(MTP_OP_GET_OBJECT_PROPS_SUPPORTED),
    LEARR(MTP_OP_GET_OBJECT_PROP_DESC),
    LEARR(MTP_OP_GET_OBJECT_PROP_VALUE),
    LEARR(MTP_OP_SET_OBJECT_PROP_VALUE),
    LEARR(MTP_OP_GET_OBJECT_PROPLIST),
    LEARR(MTP_OP_GET_OBJECT_PROP_REFERENCES),
    //events supported
    0x06,0x00,0x00,0x00,
    LEARR(MTP_EVENT_OBJECTADDED),
    LEARR(MTP_EVENT_OBJECTREMOVED),
    LEARR(MTP_EVENT_STOREADDED),
    LEARR(MTP_EVENT_STOREREMOVED),
    LEARR(MTP_EVENT_OBJECTINFOCHANGED),
    LEARR(MTP_EVENT_STORAGEINFOCHANGED),
    //device properties supported
    0x04,0x00,0x00,0x00,
    LEARR(MTP_DEV_PROP_BATTERY_LEVEL),
    LEARR(MTP_DEV_PROP_SYNCHRONIZATION_PARTNER),
    LEARR(MTP_DEV_PROP_DEVICE_FRIENDLY_NAME),
    LEARR(MTP_DEV_PROP_PERCEIVED_DEVICE_TYPE),
    //capture formats
    0x00,0x00,0x00,0x00,
    //playback formats
    0x02,0x00,0x00,0x00,
    LEARR(MTP_OF_UNDEFINED),
    LEARR(MTP_OF_ASSOCIATION),
    //manufacturer
    0x0a,
    'b',0x00,'e',0x00,'k',0x00,'e',0x00,'n',0x00,'c',0x00,'o',0x00,'r',0x00,'p',0x00,0x00,0x00,
    //model
    0x06,
    'g',0x00,'l',0x00,'a',0x00,'s',0x00,'s',0x00,0x00,0x00,
    //device version
    0x05,
    'v',0x00,'1',0x00,'.',0x00,'0',0x00,0x00,0x00,
    //serial number
    0x07,
    '1',0x00,'2',0x00,'3',0x00,'4',0x00,'5',0x00,'6',0x00,0x00,0x00,
};

static const uint8_t partner_name[] = {
    0x08,
    'D',0x00,'E',0x00,'S',0x00,'K',0x00,'T',0x00,'O',0x00,'P',0x00,0x00,0x00,
};

static const uint8_t properties_supported[] = {
    0x0d,0x00,0x00,0x00,
    LEARR(MTP_OB_PROP_STORAGE_ID),
    LEARR(MTP_OB_PROP_OBJECT_FORMAT),
    LEARR(MTP_OB_PROP_PROTECTION_STATUS),
    LEARR(MTP_OB_PROP_OBJECT_SIZE),
    LEARR(MTP_OB_PROP_OBJ_FILE_NAME),
    LEARR(MTP_OB_PROP_PARENT_OBJECT),
    LEARR(MTP_OB_PROP_PERS_UNIQ_OBJ_IDEN),
    LEARR(MTP_OB_PROP_NON_CONSUMABLE),
    LEARR(MTP_OB_PROP_DATE_MODIFIED),
    LEARR(MTP_OB_PROP_NAME),
    LEARR(MTP_OB_PROP_ASSOC_TYPE),
    LEARR(MTP_OB_PROP_DATE_CREATED),
    LEARR(MTP_OB_PROP_DRM_STATUS),
};

static uint32_t mtp_build_device_info(uint8_t *buf)
{
    const uint8_t *model;
    const uint8_t *model_end;
    uint32_t prefix_len;
    uint32_t name_len;
    uint32_t suffix_len;
    uint8_t *p;

    p = (uint8_t *)mtp_deviceinfo_dataset;
    p += 2 + 4 + 2; /* StandardVersion + VendorExtensionID + VendorExtensionVersion */
    p = (uint8_t *)mtp_skip_string(p);       /* VendorExtensionDesc */
    p += 2;                                  /* FunctionalMode */
    p = (uint8_t *)mtp_skip_u16_array(p);    /* OperationsSupported */
    p = (uint8_t *)mtp_skip_u16_array(p);    /* EventsSupported */
    p = (uint8_t *)mtp_skip_u16_array(p);    /* DevicePropertiesSupported */
    p = (uint8_t *)mtp_skip_u16_array(p);    /* CaptureFormats */
    p = (uint8_t *)mtp_skip_u16_array(p);    /* PlaybackFormats */
    p = (uint8_t *)mtp_skip_string(p);       /* Manufacturer */

    model = p;
    model_end = mtp_skip_string(model);
    prefix_len = (uint32_t)(model - mtp_deviceinfo_dataset);
    suffix_len = (uint32_t)(sizeof(mtp_deviceinfo_dataset) -
                            (uint32_t)(model_end - mtp_deviceinfo_dataset));

    if (prefix_len >= MTP_BUFFER_SIZE || suffix_len >= MTP_BUFFER_SIZE) {
        return 0;
    }

    os_memcpy(buf, mtp_deviceinfo_dataset, prefix_len);
    name_len = mtp_put_ascii_string(&buf[prefix_len], CONFIG_USBD_MTP_PRODUCT_NAME);
    if ((prefix_len + name_len + suffix_len) > MTP_BUFFER_SIZE) {
        return 0;
    }

    os_memcpy(&buf[prefix_len + name_len], model_end, suffix_len);
    return prefix_len + name_len + suffix_len;
}

static struct usb_bos_descriptor mtp_bos_descriptor = {
    .string = (uint8_t *)mtp_bos_desc,
    .string_len = 0xc,
};

/* ============================================================
 * CherryUSB v1.6 descriptor registration (callback style).
 *
 * v1.6 replaced the v0.7 monolithic raw-byte descriptor blob with a
 * struct usb_descriptor of per-kind callbacks. The glass raw mtp_descriptor[]
 * above is left in place (unused) for reference; below is the v1.6 form.
 * ============================================================ */
static const uint8_t mtp_v16_device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0200, 0x01)
};

static const uint8_t mtp_v16_config_descriptor[] = {
    USB_CONFIG_DESCRIPTOR_INIT(0x27, 0x01, 0x01, USB_CONFIG_SELF_POWERED, USBD_MAX_POWER),
    USB_INTERFACE_DESCRIPTOR_INIT(0x00, 0x00, 0x03, USB_MTP_CLASS, USB_MTP_SUB_CLASS, USB_MTP_PROTOCOL, 0x04),
#ifdef CONFIG_USB_HS
    USB_ENDPOINT_DESCRIPTOR_INIT(MTP_IN_EP, 0x02, 512, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(MTP_OUT_EP, 0x02, 512, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(MTP_INT_EP, 0x03, 28, 0x18),
#else
    USB_ENDPOINT_DESCRIPTOR_INIT(MTP_IN_EP, 0x02, 64, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(MTP_OUT_EP, 0x02, 64, 0x00),
    USB_ENDPOINT_DESCRIPTOR_INIT(MTP_INT_EP, 0x03, 28, 0x06),
#endif
};

static const uint8_t mtp_v16_device_quality_descriptor[] = {
    0x0a, USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00,
};

static const char *mtp_v16_string_descriptors[] = {
    (const char[]){ 0x09, 0x04 }, /* Langid: 0x0409 */
    "Beken",                       /* iManufacturer (index 1) */
    CONFIG_USBD_MTP_PRODUCT_NAME,   /* iProduct      (index 2) */
    "20250001",                    /* iSerialNumber (index 3) */
    "MTP",                         /* iInterface    (index 4) */
};

static const uint8_t *mtp_v16_device_descriptor_cb(uint8_t speed)
{
    (void)speed;
    return mtp_v16_device_descriptor;
}

static const uint8_t *mtp_v16_config_descriptor_cb(uint8_t speed)
{
    (void)speed;
    return mtp_v16_config_descriptor;
}

static const uint8_t *mtp_v16_device_quality_descriptor_cb(uint8_t speed)
{
    (void)speed;
    return mtp_v16_device_quality_descriptor;
}

static const char *mtp_v16_string_descriptor_cb(uint8_t speed, uint8_t index)
{
    (void)speed;
    if (index >= (sizeof(mtp_v16_string_descriptors) / sizeof(mtp_v16_string_descriptors[0]))) {
        return NULL;
    }
    return mtp_v16_string_descriptors[index];
}

static const struct usb_descriptor mtp_v16_descriptor = {
    .device_descriptor_callback = mtp_v16_device_descriptor_cb,
    .config_descriptor_callback = mtp_v16_config_descriptor_cb,
    .device_quality_descriptor_callback = mtp_v16_device_quality_descriptor_cb,
    .string_descriptor_callback = mtp_v16_string_descriptor_cb,
    .bos_descriptor = &mtp_bos_descriptor,
};

static void mtp_v16_usbd_event_handler(uint8_t busid, uint8_t event)
{
    /* Per-interface intf->notify_handler (mtp_notify_handler) is invoked by the
     * v1.6 core for bus events, so keep this global handler a no-op to avoid
     * double-handling USBD_EVENT_CONFIGURED. */
    (void)busid;
    (void)event;
}

static void mtp_env_reset(void)
{
    current_session_id = 0;
    mtp_state = MTP_STATE_IDLE;
    mtp_thread_op = MTP_THREAD_OP_NONE;
    mtp_ep_recive_cnt = 0;
    mtp_ep_recive_total_size = 0;
}

static void mtp_send_respond_ex(uint32_t transaction_id,uint16_t respond_code,uint32_t *parameter,uint8_t num)
{
    if(num > 5) return;
    mtp_packet_t *pack = (mtp_packet_t *)ep_in_buffer;
    pack->container_length = 12;
    pack->container_type = MTP_CONT_TYPE_RESPONSE;
    pack->code = respond_code;
    pack->transaction_id = transaction_id;
    if(respond_code == MTP_RSP_SESSION_ALREADY_OPEN)
    {
        pack->parameter[0] = current_session_id;
        pack->container_length += 4;
    }
    else
    {
        if(num)
        {
            for(int i = 0; i < num; i++)
            {
                pack->parameter[i] = parameter[i];
            }
            pack->container_length += 4*num;
        }
    }
    mtp_state = MTP_STATE_SEND_RSP;
    usbd_ep_start_write(mtp_ep_data[MTP_IN_EP_IDX].ep_addr,ep_in_buffer, pack->container_length);
}

static void mtp_send_respond(uint32_t transaction_id,uint16_t respond_code)
{
    mtp_send_respond_ex(transaction_id,respond_code,NULL,0);
}

static void mtp_send_event_ex(uint32_t transaction_id,uint16_t respond_code,uint32_t *parameter,uint8_t num)
{
    if(num > 3) return;
    mtp_packet_t *pack = (mtp_packet_t *)ep_in_buffer;
    pack->container_length = 12;
    pack->container_type = MTP_CONT_TYPE_EVENT;
    pack->code = respond_code;
    pack->transaction_id = transaction_id;
    if(num)
    {
        for(int i = 0; i < num; i++)
        {
            pack->parameter[i] = parameter[i];
        }
        pack->container_length += 4*num;
    }
    mtp_state = MTP_STATE_SEND_EVENT;
    usbd_ep_start_write(mtp_ep_data[MTP_INT_EP_IDX].ep_addr,ep_in_buffer, pack->container_length);
}

static void mtp_send_event(uint32_t transaction_id,uint16_t respond_code)
{
    mtp_send_event_ex(transaction_id,respond_code,NULL,0);
}

static void mtp_send_data(uint32_t transaction_id,uint16_t respond_code,uint8_t *data,uint32_t len)
{
    mtp_packet_t *pack = (mtp_packet_t *)ep_in_buffer;
    pack->container_length = 12 + len;
    pack->container_type = MTP_CONT_TYPE_DATA;
    pack->code = respond_code;
    pack->transaction_id = transaction_id;

    uint32_t cp = MAX_PACKET_SIZE-12;
    if(len < cp) cp = len;
    memcpy(pack->payload,data,cp);
    current_sending_buf = data;
    current_sending_total_size = len;
    current_sending_idx = cp;
    mtp_state = MTP_STATE_SEND_DATA;
    usbd_ep_start_write(mtp_ep_data[MTP_IN_EP_IDX].ep_addr,ep_in_buffer, 12+cp);
}

static void mtp_command_packet_handle(mtp_packet_t *pack)
{
    //USB_LOG_INFO("recive cmd:0x%04X\r\n", pack->code);
    if(!current_session_id && pack->code != MTP_OP_GET_DEVICE_INFO && pack->code != MTP_OP_OPEN_SESSION && pack->code != MTP_OP_GET_OBJECT_PROPS_SUPPORTED)
    {
        mtp_send_respond(pack->transaction_id,MTP_RSP_SESSION_NOT_OPEN);
        return;
    }
    switch(pack->code)
    {
        case MTP_OP_GET_DEVICE_INFO:
            {
                uint32_t len = mtp_build_device_info(mtp_buffer);
                if (len == 0) {
                    mtp_send_data(pack->transaction_id,pack->code,(uint8_t*)mtp_deviceinfo_dataset,sizeof(mtp_deviceinfo_dataset));
                } else {
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,len);
                }
            }
            break;
        case MTP_OP_OPEN_SESSION:
            {
                if(current_session_id)
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_SESSION_ALREADY_OPEN);
                }
                else if(pack->parameter[0] == 0)
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_PARAMETER);
                }
                else
                {
                    current_session_id = pack->parameter[0];
                    mtp_send_respond(pack->transaction_id,MTP_RSP_OK);
                }
            }
            break;
        case MTP_OP_CLOSE_SESSION:
            {
                current_session_id = 0;
                mtp_send_respond(pack->transaction_id,MTP_RSP_OK);
            }
            break;
        case MTP_OP_GET_STORAGE_IDS:
            {
                #if CONFIG_VFS
                uint32_t temp = NUM_OF_STORAGE;
                memcpy(&mtp_buffer[0],&temp,sizeof(temp));
                for(int i = 0; i < NUM_OF_STORAGE; i++)
                {
                    temp = ((i+1)<<16)|0x0001;
                    memcpy(&mtp_buffer[4+i*4],&temp,sizeof(temp));
                }
                mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,4+NUM_OF_STORAGE*4);
                #else
                memset(&mtp_buffer[0],0x00,sizeof(uint32_t));
                mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,sizeof(uint32_t));
                #endif
            }
            break;
        case MTP_OP_GET_STORAGE_INFO:
            {
                #if CONFIG_VFS
                if((pack->parameter[0] & 0xffff) != 0x0001 || ((pack->parameter[0]>>16) & 0xffff) > NUM_OF_STORAGE || !(pack->parameter[0] & 0xffff0000))
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_STORAGE_ID);
                    break;
                }
                mtp_thread_op = MTP_THREAD_OP_GET_STORAGE_INFO;
                mtp_wake_worker();
                #else
                mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_STORAGE_ID);
                #endif
            }
            break;
        case MTP_OP_GET_NUM_OBJECTS:
            {
                #if CONFIG_VFS
                if(pack->parameter[0] != 0xffffffff)
                {
                    if((pack->parameter[0] & 0xffff) != 0x0001 || ((pack->parameter[0]>>16) & 0xffff) > NUM_OF_STORAGE || !(pack->parameter[0] & 0xffff0000))
                    {
                        mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_STORAGE_ID);
                        break;
                    }
                }
                mtp_thread_op = MTP_THREAD_OP_GET_OBJECT_NUM;
                mtp_wake_worker();
                #else
                mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_STORAGE_ID);
                #endif
            }
            break;
        case MTP_OP_GET_OBJECT_HANDLES:
            {
                #if CONFIG_VFS
                if(pack->parameter[0] != 0xffffffff)
                {
                    if((pack->parameter[0] & 0xffff) != 0x0001 || ((pack->parameter[0]>>16) & 0xffff) > NUM_OF_STORAGE || !(pack->parameter[0] & 0xffff0000))
                    {
                        mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_STORAGE_ID);
                        break;
                    }
                }
                mtp_thread_op = MTP_THREAD_OP_GET_OBJECT_HANDLES;
                mtp_wake_worker();
                #else
                mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_STORAGE_ID);
                #endif
            }
            break;
        case MTP_OP_GET_OBJECT_INFO:
            {
                #if CONFIG_VFS
                mtp_thread_op = MTP_THREAD_OP_GET_OBJECT_INFO;
                mtp_wake_worker();
                #else
                mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_HANDLE);
                #endif
            }
            break;
        case MTP_OP_GET_OBJECT:
            {
                #if CONFIG_VFS
                mtp_thread_op = MTP_THREAD_OP_GET_OBJECT;
                mtp_wake_worker();
                #else
                mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_HANDLE);
                #endif
            }
            break;
        case MTP_OP_DELETE_OBJECT:
            {
                #if CONFIG_VFS
                if(!check_handle_exist(pack->parameter[0]))
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_HANDLE);
                    break;
                }
                mtp_thread_op = MTP_THREAD_OP_DELETE_OBJECT;
                mtp_wake_worker();
                #else
                mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_HANDLE);
                #endif
            }
            break;
        case MTP_OP_SEND_OBJECT_INFO:
            {
                #if CONFIG_VFS
                if(pack->parameter[0] != 0xffffffff)
                {
                    if((pack->parameter[0] & 0xffff) != 0x0001 || ((pack->parameter[0]>>16) & 0xffff) > NUM_OF_STORAGE || !(pack->parameter[0] & 0xffff0000))
                    {
                        mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_STORAGE_ID);
                        break;
                    }
                }
                if(pack->parameter[1] != 0xffffffff && !check_handle_exist(pack->parameter[1]))
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_PARENT_OBJECT);
                    break;
                }
                mtp_parameter_backup[0] = pack->parameter[0];
                mtp_parameter_backup[1] = pack->parameter[1];
                usbd_ep_start_read(mtp_ep_data[MTP_OUT_EP_IDX].ep_addr, ep_out_buffer, MAX_PACKET_SIZE);
                #else
                mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_STORAGE_ID);
                #endif
            }
            break;
        case MTP_OP_SEND_OBJECT:
            {
                mtp_parameter_backup[0] = pack->transaction_id;
                usbd_ep_start_read(mtp_ep_data[MTP_OUT_EP_IDX].ep_addr, ep_out_buffer, MAX_PACKET_SIZE);
            }
            break;
        case MTP_OP_FORMAT_STORE:
            {
                #if CONFIG_VFS
                if(pack->parameter[0] != 0xffffffff)
                {
                    if((pack->parameter[0] & 0xffff) != 0x0001 || ((pack->parameter[0]>>16) & 0xffff) > NUM_OF_STORAGE || !(pack->parameter[0] & 0xffff0000))
                    {
                        mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_STORAGE_ID);
                        break;
                    }
                }
                mtp_thread_op = MTP_THREAD_OP_FORMAT_STORAGE;
                mtp_wake_worker();
                #else
                mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_STORAGE_ID);
                #endif
            }
            break;
        case MTP_OP_GET_DEVICE_PROP_DESC:
            {
                if(pack->parameter[0] == MTP_DEV_PROP_DEVICE_FRIENDLY_NAME)
                {
                    uint32_t name_len;
                    mtp_buffer[0] = MTP_DEV_PROP_DEVICE_FRIENDLY_NAME&0xff;
                    mtp_buffer[1] = (MTP_DEV_PROP_DEVICE_FRIENDLY_NAME>>8)&0xff;
                    mtp_buffer[2] = MTP_DATATYPE_STR&0xff;
                    mtp_buffer[3] = (MTP_DATATYPE_STR>>8)&0xff;
                    mtp_buffer[4] = MTP_PROP_GET_SET;
                    int len = 5;
                    name_len = mtp_put_ascii_string(&mtp_buffer[len], CONFIG_USBD_MTP_PRODUCT_NAME);
                    len += name_len;
                    name_len = mtp_put_ascii_string(&mtp_buffer[len], CONFIG_USBD_MTP_PRODUCT_NAME);
                    len += name_len;
                    mtp_buffer[len++] = 0x00;
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,len);
                }
                else if(pack->parameter[0] == MTP_DEV_PROP_BATTERY_LEVEL)
                {
                    mtp_buffer[0] = MTP_DEV_PROP_BATTERY_LEVEL&0xff;
                    mtp_buffer[1] = (MTP_DEV_PROP_BATTERY_LEVEL>>8)&0xff;
                    mtp_buffer[2] = MTP_DATATYPE_UINT8&0xff;
                    mtp_buffer[3] = (MTP_DATATYPE_UINT8>>8)&0xff;
                    mtp_buffer[4] = MTP_PROP_GET;
                    mtp_buffer[5] = 100;

                    extern uint8_t get_battery_level(void);
                    mtp_buffer[6] = get_battery_level();

                    mtp_buffer[7] = 0x01;
                    mtp_buffer[8] = 0;
                    mtp_buffer[9] = 100;
                    mtp_buffer[10] = 1;
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,11);
                }
                else if(pack->parameter[0] == MTP_DEV_PROP_PERCEIVED_DEVICE_TYPE)
                {
                    uint32_t device_type = mtp_get_perceived_device_type();
                    mtp_buffer[0] = MTP_DEV_PROP_PERCEIVED_DEVICE_TYPE&0xff;
                    mtp_buffer[1] = (MTP_DEV_PROP_PERCEIVED_DEVICE_TYPE>>8)&0xff;
                    mtp_buffer[2] = MTP_DATATYPE_UINT32&0xff;
                    mtp_buffer[3] = (MTP_DATATYPE_UINT32>>8)&0xff;
                    mtp_buffer[4] = MTP_PROP_GET;
                    memcpy(&mtp_buffer[5],&device_type,sizeof(device_type));
                    memcpy(&mtp_buffer[9],&device_type,sizeof(device_type));
                    mtp_buffer[13] = 0x00;
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,14);
                }
                else if(pack->parameter[0] == MTP_DEV_PROP_SYNCHRONIZATION_PARTNER)
                {
                    mtp_buffer[0] = MTP_DEV_PROP_SYNCHRONIZATION_PARTNER&0xff;
                    mtp_buffer[1] = (MTP_DEV_PROP_SYNCHRONIZATION_PARTNER>>8)&0xff;
                    mtp_buffer[2] = MTP_DATATYPE_STR&0xff;
                    mtp_buffer[3] = (MTP_DATATYPE_STR>>8)&0xff;
                    mtp_buffer[4] = MTP_PROP_GET_SET;
                    int len = 5;
                    memcpy(&mtp_buffer[len],partner_name,sizeof(partner_name));
                    len += sizeof(partner_name);
                    memcpy(&mtp_buffer[len],partner_name,sizeof(partner_name));
                    len += sizeof(partner_name);
                    mtp_buffer[len++] = 0x00;
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,len);
                }
                else
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_PROP_NOT_SUPPORTED);
                }
            }
            break;
        case MTP_OP_GET_DEVICE_PROP_VALUE:
            {
                if(pack->parameter[0] == MTP_DEV_PROP_DEVICE_FRIENDLY_NAME)
                {
                    uint32_t len = mtp_put_ascii_string(mtp_buffer, CONFIG_USBD_MTP_PRODUCT_NAME);
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,len);
                }
                else if(pack->parameter[0] == MTP_DEV_PROP_BATTERY_LEVEL)
                {
                    extern uint8_t get_battery_level(void);
                    mtp_buffer[0] = get_battery_level();

                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,1);
                }
                else if(pack->parameter[0] == MTP_DEV_PROP_PERCEIVED_DEVICE_TYPE)
                {
                    uint32_t device_type = mtp_get_perceived_device_type();
                    memcpy(&mtp_buffer[0],&device_type,sizeof(device_type));
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,4);
                }
                else if(pack->parameter[0] == MTP_DEV_PROP_SYNCHRONIZATION_PARTNER)
                {
                    memcpy(&mtp_buffer[0],partner_name,sizeof(partner_name));
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,sizeof(partner_name));
                }
                else
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_PROP_NOT_SUPPORTED);
                }
            }
            break;
        case MTP_OP_MOVE_OBJECT:
            {
                #if CONFIG_VFS
                if((pack->parameter[1] & 0xffff) != 0x0001 || ((pack->parameter[1]>>16) & 0xffff) > NUM_OF_STORAGE || !(pack->parameter[1] & 0xffff0000))
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_STORAGE_ID);
                    break;
                }
                mtp_thread_op = MTP_THREAD_OP_MOVE_OBJECT;
                mtp_wake_worker();
                #else
                mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                #endif
            }
            break;
        case MTP_OP_COPY_OBJECT:
            {
                #if CONFIG_VFS
                if((pack->parameter[1] & 0xffff) != 0x0001 || ((pack->parameter[1]>>16) & 0xffff) > NUM_OF_STORAGE || !(pack->parameter[1] & 0xffff0000))
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_STORAGE_ID);
                    break;
                }
                mtp_thread_op = MTP_THREAD_OP_COPY_OBJECT;
                mtp_wake_worker();
                #else
                mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                #endif
            }
            break;
        case MTP_OP_GET_PARTIAL_OBJECT:
            {
                #if CONFIG_VFS
                mtp_thread_op = MTP_THREAD_OP_GET_OBJECT;
                mtp_wake_worker();
                #else
                mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_HANDLE);
                #endif
            }
            break;
        case MTP_OP_GET_OBJECT_PROPS_SUPPORTED:
            {
                //USB_LOG_INFO("MTP_OP_GET_OBJECT_PROPS_SUPPORTED:0x%04X\r\n", pack->parameter[0]);
                if(pack->parameter[0] == MTP_OF_UNDEFINED || pack->parameter[0] == MTP_OF_ASSOCIATION)
                {
                    mtp_send_data(pack->transaction_id,pack->code,(uint8_t*)properties_supported,sizeof(properties_supported));
                }
                else
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_FORMAT_CODE);
                }
            }
            break;
        case MTP_OP_GET_OBJECT_PROP_DESC:
            {
                //USB_LOG_ERR("MTP_OP_GET_OBJECT_PROP_DESC:0x%08X,0x%08X\r\n",pack->parameter[0],pack->parameter[1]);
                if(pack->parameter[0] == MTP_OB_PROP_STORAGE_ID)
                {
                    mtp_buffer[0] = MTP_OB_PROP_STORAGE_ID&0xff;
                    mtp_buffer[1] = (MTP_OB_PROP_STORAGE_ID>>8)&0xff;
                    mtp_buffer[2] = MTP_DATATYPE_UINT32&0xff;
                    mtp_buffer[3] = (MTP_DATATYPE_UINT32>>8)&0xff;
                    mtp_buffer[4] = MTP_PROP_GET;
                    memset(&mtp_buffer[5],0x00,9);
                    mtp_buffer[9] = 0x08;
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,14);
                }
                else if(pack->parameter[0] == MTP_OB_PROP_OBJECT_FORMAT)
                {
                    mtp_buffer[0] = MTP_OB_PROP_OBJECT_FORMAT&0xff;
                    mtp_buffer[1] = (MTP_OB_PROP_OBJECT_FORMAT>>8)&0xff;
                    mtp_buffer[2] = MTP_DATATYPE_UINT16&0xff;
                    mtp_buffer[3] = (MTP_DATATYPE_UINT16>>8)&0xff;
                    mtp_buffer[4] = MTP_PROP_GET;
                    mtp_buffer[5] = 0x00;
                    mtp_buffer[6] = 0x30;
                    mtp_buffer[7] = 0x04;
                    memset(&mtp_buffer[8],0x00,4);
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,12);
                }
                else if(pack->parameter[0] == MTP_OB_PROP_PROTECTION_STATUS)
                {
                    const uint8_t protect_enum[] = {0x04,0x00,0x00,0x00,0x01,0x00,0x02,0x80,0x03,0x80};
                    mtp_buffer[0] = MTP_OB_PROP_PROTECTION_STATUS&0xff;
                    mtp_buffer[1] = (MTP_OB_PROP_PROTECTION_STATUS>>8)&0xff;
                    mtp_buffer[2] = MTP_DATATYPE_UINT16&0xff;
                    mtp_buffer[3] = (MTP_DATATYPE_UINT16>>8)&0xff;
                    mtp_buffer[4] = MTP_PROP_GET;
                    mtp_buffer[5] = 0x00;
                    mtp_buffer[6] = 0x00;
                    mtp_buffer[7] = 0x08;
                    memset(&mtp_buffer[8],0x00,3);
                    mtp_buffer[11] = 0x02;
                    memcpy(&mtp_buffer[12],protect_enum,sizeof(protect_enum));
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,12+sizeof(protect_enum));
                }
                else if(pack->parameter[0] == MTP_OB_PROP_OBJECT_SIZE)
                {
                    mtp_buffer[0] = MTP_OB_PROP_OBJECT_SIZE&0xff;
                    mtp_buffer[1] = (MTP_OB_PROP_OBJECT_SIZE>>8)&0xff;
                    mtp_buffer[2] = MTP_DATATYPE_UINT64&0xff;
                    mtp_buffer[3] = (MTP_DATATYPE_UINT64>>8)&0xff;
                    mtp_buffer[4] = MTP_PROP_GET;
                    memset(&mtp_buffer[5],0x00,13);
                    mtp_buffer[13] = 0x04;
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,18);
                }
                else if(pack->parameter[0] == MTP_OB_PROP_ASSOC_TYPE)
                {
                    const uint8_t assoc_enum[] = {0x02,0x00,0x00,0x00,0x01,0x00};
                    mtp_buffer[0] = MTP_OB_PROP_ASSOC_TYPE&0xff;
                    mtp_buffer[1] = (MTP_OB_PROP_ASSOC_TYPE>>8)&0xff;
                    mtp_buffer[2] = MTP_DATATYPE_UINT16&0xff;
                    mtp_buffer[3] = (MTP_DATATYPE_UINT16>>8)&0xff;
                    mtp_buffer[4] = MTP_PROP_GET_SET;
                    memset(&mtp_buffer[5],0x00,6);
                    mtp_buffer[7] = 0x08;
                    mtp_buffer[11] = 0x02;
                    memcpy(&mtp_buffer[12],assoc_enum,sizeof(assoc_enum));
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,12+sizeof(assoc_enum));
                }
                else if(pack->parameter[0] == MTP_OB_PROP_OBJ_FILE_NAME || pack->parameter[0] == MTP_OB_PROP_NAME)
                {
                    const uint8_t defualt_name[] = {0x08,'N',0x00,'o',0x00,' ',0x00,'N',0x00,'a',0x00,'m',0x00,'e',0x00,0x00,0x00};
                    unsigned char regular[] = {0x5, 0x57, 0x5b, 0x0, 0x61, 0x0, 0x2d, 0x0, 0x7a, 0x0, 0x41, 0x0, 0x2d, 0x0, 0x5a, 
                                                0x0, 0x21, 0x0, 0x23, 0x0, 0x5c, 0x0, 0x24, 0x0, 0x25, 0x0, 0x26, 0x0, 0x60, 0x0, 
                                                0x5c, 0x0, 0x28, 0x0, 0x5c, 0x0, 0x29, 0x0, 0x5c, 0x0, 0x2d, 0x0, 0x30, 0x0, 0x2d, 
                                                0x0, 0x39, 0x0, 0x40, 0x0, 0x5c, 0x0, 0x5e, 0x0, 0x5f, 0x0, 0x5c, 0x0, 0x27, 0x0, 
                                                0x5c, 0x0, 0x7b, 0x0, 0x5c, 0x0, 0x7d, 0x0, 0x5c, 0x0, 0x7e, 0x0, 0x5d, 0x0, 0x7b, 
                                                0x0, 0x31, 0x0, 0x2c, 0x0, 0x38, 0x0, 0x7d, 0x0, 0x5c, 0x0, 0x2e, 0x0, 0x5b, 0x0, 
                                                0x5b, 0x0, 0x61, 0x0, 0x2d, 0x0, 0x7a, 0x0, 0x41, 0x0, 0x2d, 0x0, 0x5a, 0x0, 0x21, 
                                                0x0, 0x23, 0x0, 0x5c, 0x0, 0x24, 0x0, 0x25, 0x0, 0x26, 0x0, 0x60, 0x0, 0x5c, 0x0, 
                                                0x28, 0x0, 0x5c, 0x0, 0x29, 0x0, 0x5c, 0x0, 0x2d, 0x0, 0x30, 0x0, 0x2d, 0x0, 0x39, 
                                                0x0, 0x40, 0x0, 0x5c, 0x0, 0x5e, 0x0, 0x5f, 0x0, 0x5c, 0x0, 0x27, 0x0, 0x5c, 0x0, 
                                                0x7b, 0x0, 0x5c, 0x0, 0x7d, 0x0, 0x5c, 0x0, 0x7e, 0x0, 0x5d, 0x0, 0x7b, 0x0, 0x31, 
                                                0x0, 0x2c, 0x0, 0x33, 0x0, 0x7d, 0x0, 0x5d, 0x0, 0x0, 0x0};
                    mtp_buffer[0] = pack->parameter[0]&0xff;
                    mtp_buffer[1] = (pack->parameter[0]>>8)&0xff;
                    mtp_buffer[2] = MTP_DATATYPE_STR&0xff;
                    mtp_buffer[3] = (MTP_DATATYPE_STR>>8)&0xff;
                    mtp_buffer[4] = MTP_PROP_GET_SET;
                    memcpy(&mtp_buffer[5],defualt_name,sizeof(defualt_name));
                    int idx = 5 + sizeof(defualt_name);
                    if(pack->parameter[0] == MTP_OB_PROP_OBJ_FILE_NAME)
                    {
                        mtp_buffer[idx++] = 0x04;
                        mtp_buffer[idx++] = 0x00;
                        mtp_buffer[idx++] = 0x00;
                        mtp_buffer[idx++] = 0x00;
                        memcpy(&mtp_buffer[idx],regular,sizeof(regular));
                        idx += sizeof(regular);
                    }
                    else
                    {
                        mtp_buffer[idx++] = 0x08;
                        mtp_buffer[idx++] = 0x00;
                        mtp_buffer[idx++] = 0x00;
                        mtp_buffer[idx++] = 0x00;
                        mtp_buffer[idx++] = 0x00;
                    }
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,idx);
                }
                else if(pack->parameter[0] == MTP_OB_PROP_DATE_CREATED || pack->parameter[0] == MTP_OB_PROP_DATE_MODIFIED)
                {
                    mtp_buffer[0] = pack->parameter[0]&0xff;
                    mtp_buffer[1] = (pack->parameter[0]>>8)&0xff;
                    mtp_buffer[2] = MTP_DATATYPE_STR&0xff;
                    mtp_buffer[3] = (MTP_DATATYPE_STR>>8)&0xff;
                    mtp_buffer[4] = MTP_PROP_GET;
                    memset(&mtp_buffer[5],0x00,5);
                    mtp_buffer[6] = 0x08;
                    mtp_buffer[10] = 0x03;
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,11);
                }
                else if(pack->parameter[0] == MTP_OB_PROP_PARENT_OBJECT)
                {
                    mtp_buffer[0] = pack->parameter[0]&0xff;
                    mtp_buffer[1] = (pack->parameter[0]>>8)&0xff;
                    mtp_buffer[2] = MTP_DATATYPE_UINT32&0xff;
                    mtp_buffer[3] = (MTP_DATATYPE_UINT32>>8)&0xff;
                    mtp_buffer[4] = MTP_PROP_GET;
                    memset(&mtp_buffer[5],0x00,9);
                    mtp_buffer[9] = 0x08;
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,14);
                }
                else if(pack->parameter[0] == MTP_OB_PROP_PERS_UNIQ_OBJ_IDEN)
                {
                    mtp_buffer[0] = MTP_OB_PROP_PERS_UNIQ_OBJ_IDEN&0xff;
                    mtp_buffer[1] = (MTP_OB_PROP_PERS_UNIQ_OBJ_IDEN>>8)&0xff;
                    mtp_buffer[2] = MTP_DATATYPE_UINT128&0xff;
                    mtp_buffer[3] = (MTP_DATATYPE_UINT128>>8)&0xff;
                    mtp_buffer[4] = MTP_PROP_GET_SET;
                    memset(&mtp_buffer[5],0x00,21);
                    mtp_buffer[21] = 0x04;
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,26);
                }
                else if(pack->parameter[0] == MTP_OB_PROP_NON_CONSUMABLE)
                {
                    const uint8_t consumable_enum[] = {0x02,0x00,0x00,0x01};
                    mtp_buffer[0] = MTP_OB_PROP_NON_CONSUMABLE&0xff;
                    mtp_buffer[1] = (MTP_OB_PROP_NON_CONSUMABLE>>8)&0xff;
                    mtp_buffer[2] = MTP_DATATYPE_UINT8&0xff;
                    mtp_buffer[3] = (MTP_DATATYPE_UINT8>>8)&0xff;
                    mtp_buffer[4] = MTP_PROP_GET;
                    memset(&mtp_buffer[5],0x00,5);
                    mtp_buffer[6] = 0x08;
                    mtp_buffer[10] = 0x02;
                    memcpy(&mtp_buffer[11],consumable_enum,sizeof(consumable_enum));
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,11+sizeof(consumable_enum));
                }
                else if(pack->parameter[0] == MTP_OB_PROP_DRM_STATUS)
                {
                    const uint8_t drm_enum[] = {0x02,0x00,0x00,0x00,0x01,0x00};
                    mtp_buffer[0] = MTP_OB_PROP_DRM_STATUS&0xff;
                    mtp_buffer[1] = (MTP_OB_PROP_DRM_STATUS>>8)&0xff;
                    mtp_buffer[2] = MTP_DATATYPE_UINT16&0xff;
                    mtp_buffer[3] = (MTP_DATATYPE_UINT16>>8)&0xff;
                    mtp_buffer[4] = MTP_PROP_GET;
                    memset(&mtp_buffer[5],0x00,6);
                    mtp_buffer[7] = 0x08;
                    mtp_buffer[11] = 0x02;
                    memcpy(&mtp_buffer[12],drm_enum,sizeof(drm_enum));
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,12+sizeof(drm_enum));
                }
                else
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_PROP_CODE);
                }
            }
            break;
        case MTP_OP_GET_OBJECT_PROP_VALUE:
            {
                //USB_LOG_ERR("MTP_OP_GET_OBJECT_PROP_VALUE:0x%08X,0x%08X\r\n",pack->parameter[0],pack->parameter[1]);
                if(pack->parameter[0] == MTP_OB_PROP_STORAGE_ID)
                {
                    #if CONFIG_VFS
                    handle_map_item_t *item = get_list_item_by_handle(pack->parameter[0]);
                    if(item == NULL)
                    {
                        mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_HANDLE);
                        break;
                    }
                    uint32_t storage_id = (item->storage_id<<16) | 0x0001;;
                    memcpy(mtp_buffer,&storage_id,sizeof(uint32_t));
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,sizeof(uint32_t));
                    #else
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_PROP_CODE);
                    #endif
                }
                else if(pack->parameter[1] == MTP_OB_PROP_OBJECT_FORMAT)
                {
                    handle_map_item_t *item = get_list_item_by_handle(pack->parameter[0]);
                    if(item == NULL)
                    {
                        mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_HANDLE);
                        break;
                    }
                    if(item->is_dir)
                    {
                        mtp_buffer[0] = MTP_OF_ASSOCIATION&0xff;
                        mtp_buffer[1] = (MTP_OF_ASSOCIATION>>8)&0xff;
                    }
                    else
                    {
                        mtp_buffer[0] = MTP_OF_UNDEFINED&0xff;
                        mtp_buffer[1] = (MTP_OF_UNDEFINED>>8)&0xff;
                    }
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,2);
                }
                else if(pack->parameter[1] == MTP_OB_PROP_PROTECTION_STATUS)
                {
                    mtp_buffer[0] = 0x00;
                    mtp_buffer[1] = 0x00;
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,2);
                }
                else if(pack->parameter[1] == MTP_OB_PROP_OBJECT_SIZE)
                {
                    #if CONFIG_VFS
                    mtp_thread_op = MTP_THREAD_OP_GET_OBJECT_SIZE;
                    mtp_wake_worker();
                    #else
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_PROP_CODE);
                    #endif
                }
                else if(pack->parameter[1] == MTP_OB_PROP_ASSOC_TYPE)
                {
                    handle_map_item_t *item = get_list_item_by_handle(pack->parameter[0]);
                    if(item == NULL)
                    {
                        mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_HANDLE);
                        break;
                    }
                    if(item->is_dir)
                    {
                        mtp_buffer[0] = 0x01;
                        mtp_buffer[1] = 0x00;
                    }
                    else
                    {
                        mtp_buffer[0] = 0x00;
                        mtp_buffer[1] = 0x00;
                    }
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,2);
                }
                else if(pack->parameter[1] == MTP_OB_PROP_OBJ_FILE_NAME || pack->parameter[1] == MTP_OB_PROP_NAME)
                {
                    #if CONFIG_VFS
                    mtp_thread_op = MTP_THREAD_OP_GET_OBJECT_FILE_NAME;
                    mtp_wake_worker();
                    #else
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_PROP_CODE);
                    #endif
                }
                else if(pack->parameter[1] == MTP_OB_PROP_PARENT_OBJECT)
                {
                    handle_map_item_t *item = get_list_item_by_handle(pack->parameter[0]);
                    memcpy(mtp_buffer,&item->parent_handle,4);
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,4);
                }
                else if(pack->parameter[1] == MTP_OB_PROP_PERS_UNIQ_OBJ_IDEN)
                {
                    memset(mtp_buffer,0x00,16);
                    memcpy(mtp_buffer,&pack->parameter[0],4);
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,16);
                }
                else if(pack->parameter[1] == MTP_OB_PROP_DRM_STATUS)
                {
                    mtp_buffer[0] = 0x00;
                    mtp_buffer[1] = 0x00;
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,2);
                }
                else if(pack->parameter[1] == MTP_OB_PROP_DATE_CREATED || pack->parameter[1] == MTP_OB_PROP_DATE_MODIFIED)
                {
                    int strlen = Gb2312ToUtf16LE("20250101T000000",mtp_buffer);
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,strlen);
                }
                else if(pack->parameter[1] == MTP_OB_PROP_NON_CONSUMABLE)
                {
                    mtp_buffer[0] = 0x00;
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,1);
                }
                else
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_PROP_CODE);
                }
            }
            break;
        case MTP_OP_SET_OBJECT_PROP_VALUE:
            {
                //USB_LOG_ERR("MTP_OP_SET_OBJECT_PROP_VALUE:0x%08X,0x%08X\r\n",pack->parameter[0],pack->parameter[1]);
                #if CONFIG_VFS
                handle_map_item_t *item = get_list_item_by_handle(pack->parameter[0]);
                if(item == NULL)
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_HANDLE);
                    break;
                }
                if(pack->parameter[1] == MTP_OB_PROP_OBJ_FILE_NAME || pack->parameter[1] == MTP_OB_PROP_NAME)
                {
                    mtp_parameter_backup[0] = pack->parameter[0];
                    mtp_parameter_backup[1] = pack->parameter[1];
                    usbd_ep_start_read(mtp_ep_data[MTP_OUT_EP_IDX].ep_addr, ep_out_buffer, MAX_PACKET_SIZE);
                }
                else
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_PROP_CODE);
                }
                #else
                mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_PROP_CODE);
                #endif
            }
            break;
        case MTP_OP_GET_OBJECT_PROPLIST:
        {
            #if CONFIG_VFS
            handle_map_item_t *item = get_list_item_by_handle(pack->parameter[0]);
            if(item == NULL)
            {
                mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_HANDLE);
                break;
            }
            if(pack->parameter[2] || (pack->parameter[3] != 0x00000004 && pack->parameter[3] != 0x00000008))
            {
                mtp_send_respond(pack->transaction_id,MTP_RSP_PARAMETER_NOT_SUPPORTED);
                break;
            }
            mtp_thread_op = MTP_THREAD_OP_GET_OBJECT_PROPLIST;
            mtp_wake_worker();
            #else
            mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_PROP_CODE);
            #endif
        }
        break;
        case MTP_OP_GET_OBJECT_PROP_REFERENCES:
            {
                memset(mtp_buffer,0x00,4);
                mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,4);
            }
            break;
        default:
            mtp_send_respond(pack->transaction_id,MTP_RSP_OPERATION_NOT_SUPPORTED);
            break;
    }
}

static void mtp_data_packet_handle(mtp_packet_t *pack)
{
    //USB_LOG_INFO("recive data:0x%04X\r\n", pack->code);
    switch(pack->code)
    {
        case MTP_OP_SET_OBJECT_PROP_VALUE:
        {
            #if CONFIG_VFS
            mtp_thread_op = MTP_THREAD_OP_SET_OBJECT_PROP_VALUE;
            mtp_wake_worker();
            #endif
        }
        break;
        case MTP_OP_SEND_OBJECT_INFO:
        {
            #if CONFIG_VFS
            mtp_thread_op = MTP_THREAD_OP_SEND_OBJECT_INFO;
            mtp_wake_worker();
            #endif
        }
        break;
        default:break;
    }
}

static int mtp_class_interface_request_handler(uint8_t busid, struct usb_setup_packet *setup, uint8_t **data, uint32_t *len)
{
    (void)busid;
    USB_LOG_ERR("MTP Class request: "
                "bRequest 0x%02x\r\n",
                setup->bRequest);

    switch (setup->bRequest) {
        case MTP_REQUEST_CANCEL:
            mtp_thread_op = MTP_THREAD_OP_CANCEL_REQUEST;
            mtp_wake_worker();
            break;
        case MTP_REQUEST_GET_EXT_EVENT_DATA:
            break;
        case MTP_REQUEST_RESET:
            mtp_thread_op = MTP_THREAD_OP_RESET;
            mtp_wake_worker();
            break;
        case MTP_REQUEST_GET_DEVICE_STATUS:
            {
                uint16_t status;
                *len = 8;
                (*data)[0] = 0x08;
                (*data)[1] = 0x00;
                if(mtp_thread_op == MTP_THREAD_OP_CANCEL_REQUEST
                   || s_mtp_busy || mtp_ep_recive_total_size != 0
#if CONFIG_VFS
                   || open_fd >= 0
#endif
                   )
                {
                    status = MTP_RSP_DEVICE_BUSY;
                }
                else
                {
                    status = MTP_RSP_OK;
                    mtp_ep_recive_total_size = 0;
                    usbd_ep_start_read(mtp_ep_data[MTP_OUT_EP_IDX].ep_addr, ep_out_buffer, MAX_PACKET_SIZE);
                    mtp_state = MTP_STATE_IDLE;
                }
                (*data)[2] = status & 0xff;
                (*data)[3] = (status >> 8) & 0xff;
                (*data)[4] = 0x00;
                (*data)[5] = 0x00;
                (*data)[6] = 0x00;
                (*data)[7] = 0x00;
                return 1;
            }
            break;
        default:
            USB_LOG_DBG("Unhandled MTP Class bRequest 0x%02x\r\n", setup->bRequest);
            return -1;
    }

    return 0;
}

static void mtp_notify_handler(uint8_t busid, uint8_t event, void *arg)
{
    (void)busid;
    USB_LOG_ERR("mtp_notify_handler:%d\r\n",event);
    switch (event) {
        case USBD_EVENT_ERROR:
        case USBD_EVENT_RESET:
            USB_LOG_DBG("%s ,line:%d,USBD_EVENT_RESET\r\n",__FILE__,__LINE__);
            break;
        case USBD_EVENT_CONFIGURED:
            usbd_ep_start_read(mtp_ep_data[MTP_OUT_EP_IDX].ep_addr, ep_out_buffer, MAX_PACKET_SIZE);
            break;
        case USBD_EVENT_SUSPEND:
            mtp_thread_op = MTP_THREAD_OP_RESET;
            mtp_wake_worker();
            USB_LOG_DBG("%s ,line:%d,USBD_EVENT_SUSPEND\r\n",__FILE__,__LINE__);
            break;
        case USBD_EVENT_RESUME:
            USB_LOG_DBG("%s ,line:%d,USBD_EVENT_RESUME\r\n",__FILE__,__LINE__);
            break;
        default:
            break;
    }
}

static void mtp_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid; (void)ep; (void)nbytes;
    if(mtp_state == MTP_STATE_SEND_RSP)
    {
        usbd_ep_start_read(mtp_ep_data[MTP_OUT_EP_IDX].ep_addr, ep_out_buffer, MAX_PACKET_SIZE);
    }
    else if(mtp_state == MTP_STATE_SEND_DATA)
    {
        if(current_sending_idx < current_sending_total_size)
        {
            uint32_t cp = MAX_PACKET_SIZE;
            if(current_sending_total_size-current_sending_idx < cp) cp = current_sending_total_size-current_sending_idx;
            memcpy(ep_in_buffer,&current_sending_buf[current_sending_idx],cp);
            current_sending_idx += cp;
            usbd_ep_start_write(mtp_ep_data[MTP_IN_EP_IDX].ep_addr,ep_in_buffer, cp);
        }
        else
        {
            mtp_packet_t *pack = (mtp_packet_t*)ep_out_buffer;
            mtp_send_respond(pack->transaction_id,MTP_RSP_OK);
        }
    }
    else if(mtp_state == MTP_STATE_SEND_OBJ_DATA)
    {
        if(open_fd < 0)
        {
            mtp_send_respond(mtp_parameter_backup[0],MTP_RSP_OK);
        }
        else
        {
            mtp_thread_op = MTP_THREAD_OP_GET_OBJECT;
            mtp_wake_worker();
        }
    }
}

/* Coalesced OUT receive size for the SendObject data phase. The MUSB device
 * port (usb_dc_beken_musb_mhdrc.c) accumulates multiple bulk packets into the
 * supplied buffer and only fires the OUT-complete callback on a short packet or
 * when the requested length is filled. Arming one large read (instead of one
 * per 512-byte packet) lets the worker write a big block to the SD card in a
 * single write(), which is what makes a large-file copy fast enough to beat the
 * Windows WPD host timeout (the slow 512B-at-a-time path capped throughput at
 * ~250KB/s and the host aborted mid-transfer -> device-notify chime). */
#define MTP_RX_CHUNK (32 * 1024)

/* Size of the next OUT read. During an object data phase we know exactly how
 * many bytes remain, so we request min(chunk, remaining): the read then always
 * completes deterministically (either a short/last packet or xfer_len hits 0),
 * so a file whose size is an exact multiple of the packet size never stalls
 * waiting for bytes that will not come. Outside the data phase the size is
 * unknown, so fall back to a single max packet. */
static uint32_t mtp_next_rx_len(void)
{
    if(mtp_state == MTP_STATE_RECIVE_OBJ_DATA
       && mtp_ep_recive_total_size > mtp_ep_recive_cnt)
    {
        uint32_t remain = mtp_ep_recive_total_size - mtp_ep_recive_cnt;
        return (remain > MTP_RX_CHUNK) ? MTP_RX_CHUNK : remain;
    }
    return MAX_PACKET_SIZE;
}

static void mtp_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid; (void)ep;
    mtp_packet_t *pack = (mtp_packet_t*)ep_out_buffer;

    mtp_ep_recive_cnt += nbytes;
    //USB_LOG_INFO("mtp recive:%d,%d\r\n",mtp_ep_recive_cnt,nbytes);
    if(mtp_ep_recive_total_size == 0)
    {
        mtp_ep_recive_total_size = pack->container_length;
        if(pack->container_type == MTP_CONT_TYPE_DATA && pack->code == MTP_OP_SEND_OBJECT)
        {
            mtp_state = MTP_STATE_RECIVE_OBJ_DATA;
            if(nbytes>12)
            {
                mtp_parameter_backup[1] = (uint32_t)pack->payload;
                mtp_parameter_backup[2] = nbytes-12;
                if(mtp_ep_recive_cnt >= mtp_ep_recive_total_size)
                {
                    mtp_parameter_backup[3] = 1;
                    mtp_ep_recive_cnt = 0;
                    mtp_ep_recive_total_size = 0;
                }
                else
                {
                    mtp_parameter_backup[3] = 0;
                }
                mtp_thread_op = MTP_THREAD_OP_SEND_OBJECT;
                mtp_wake_worker();
            }
            else if(mtp_ep_recive_cnt >= mtp_ep_recive_total_size)
            {
                /* 0-byte object: the SendObject data phase is a header-only
                 * container (container_length==12, no payload). The file was
                 * already created empty by SendObjectInfo, so finish the
                 * transaction now with OK. Without this the worker is never
                 * woken and the host hangs forever waiting for the response
                 * (deterministic 0-byte file copy deadlock). mtp_bulk_in re-arms
                 * OUT once the SEND_RSP write completes, so the next object in a
                 * batch proceeds normally. */
                mtp_ep_recive_cnt = 0;
                mtp_ep_recive_total_size = 0;
                mtp_send_respond(mtp_parameter_backup[0], MTP_RSP_OK);
            }
            else
            {
                usbd_ep_start_read(mtp_ep_data[MTP_OUT_EP_IDX].ep_addr, ep_out_buffer, mtp_next_rx_len());
            }
            return;
        }
    }
    if(mtp_state == MTP_STATE_RECIVE_OBJ_DATA)
    {
        mtp_parameter_backup[1] = (uint32_t)ep_out_buffer;
        mtp_parameter_backup[2] = nbytes;
        if(mtp_ep_recive_cnt >= mtp_ep_recive_total_size)
        {
            mtp_parameter_backup[3] = 1;
            mtp_ep_recive_cnt = 0;
            mtp_ep_recive_total_size = 0;
        }
        else
        {
            mtp_parameter_backup[3] = 0;
        }
        mtp_thread_op = MTP_THREAD_OP_SEND_OBJECT;
        mtp_wake_worker();
        return;
    }
    else if(mtp_ep_recive_total_size > MAX_PACKET_SIZE)
    {
        usbd_ep_start_read(mtp_ep_data[MTP_OUT_EP_IDX].ep_addr, ep_out_buffer, MAX_PACKET_SIZE);
        USB_LOG_ERR("mtp recive err packet size:%d,%d\r\n",mtp_ep_recive_total_size,nbytes);
        if(mtp_ep_recive_cnt >= mtp_ep_recive_total_size)
        {
            mtp_ep_recive_cnt = 0;
            mtp_ep_recive_total_size = 0;
        }
        return;
    }
    if(mtp_ep_recive_total_size && mtp_ep_recive_cnt >= mtp_ep_recive_total_size)
    {
        switch(pack->container_type)
        {
            case MTP_CONT_TYPE_COMMAND:
                mtp_command_packet_handle(pack);
                break;
            case MTP_CONT_TYPE_DATA:
                mtp_data_packet_handle(pack);
                break;
            case MTP_CONT_TYPE_RESPONSE:
                break;
            case MTP_CONT_TYPE_EVENT:
                break;
            default:
                USB_LOG_ERR("mtp undefine packet type:%d\r\n",pack->container_type);
                break;
        }
        mtp_ep_recive_cnt = 0;
        mtp_ep_recive_total_size = 0;
    }
    else
    {
        usbd_ep_start_read(mtp_ep_data[MTP_OUT_EP_IDX].ep_addr, &ep_out_buffer[mtp_ep_recive_cnt], MAX_PACKET_SIZE-mtp_ep_recive_cnt);
    }
}

static void mtp_int_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid; (void)ep; (void)nbytes;
}

static void usbd_mtp_thread(void *argument)
{
    while (1) {
        uintptr_t _mqmsg = 0;
        /* Finite timeout so the worker periodically re-checks the exit request
         * even if a wakeup is ever missed; a genuine request arrives as a queue
         * message. */
        int _rq = usb_osal_mq_recv(mtp_op_q, &_mqmsg, 200);
        if(mtp_should_exit || mtp_thread_op == MTP_THREAD_EXIT)
            break;
        if(_rq != 0)
            continue; /* timeout, nothing queued */
        s_mtp_busy = 1;
        #if CONFIG_VFS
        switch(mtp_thread_op)
        {
            case MTP_THREAD_OP_GET_STORAGE_INFO:
            {
                mtp_packet_t *pack = (mtp_packet_t*)ep_out_buffer;
                uint16_t idx = ((pack->parameter[0]>>16) & 0xffff)-1;
                mtp_storageinfo_dataset_t *storage_ds = (mtp_storageinfo_dataset_t*)mtp_buffer;
                storage_ds->storage_type = 0x0003;
                storage_ds->filesystem_type = 0x0002;
                storage_ds->access_capability = 0x0000;

                struct statfs sta;
                memset(&sta,0,sizeof(sta));
                statfs(storage_info[idx].mount_path,&sta);
                storage_ds->max_capacity = (uint64_t)sta.f_blocks*sta.f_bsize;
                storage_ds->free_space_in_bytes = (uint64_t)sta.f_bfree*sta.f_bsize;
                storage_ds->free_space_in_objects = 0xffffffff;

                int str1len = storage_info[idx].name[0]*2+1;
                int str2len = storage_info[idx].identifier[0]*2+1;
                memcpy(storage_ds->storage_description_str,storage_info[idx].name,str1len);
                memcpy(&storage_ds->storage_description_str[str1len],storage_info[idx].identifier,str2len);
                mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,sizeof(mtp_storageinfo_dataset_t)+str1len+str2len);
                
            }
            break;
            case MTP_THREAD_OP_GET_OBJECT_NUM:
            {
                mtp_packet_t *pack = (mtp_packet_t*)ep_out_buffer;
                handle_map_item_t *item;
                uint32_t objectcnt = 0;
                if(pack->parameter[2] != 0 && pack->parameter[2] != 0xffffffff)
                {
                    //check if folder is exist
                    item = get_list_item_by_handle(pack->parameter[2]);
                    if(item == NULL || !item->is_dir)
                    {
                        mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_HANDLE);
                        break;
                    }
                }
                if(pack->parameter[2] == 0xffffffff)
                {
                    //search root
                    if(pack->parameter[0] == 0xffffffff)
                    {
                        //search all storage
                        for(int i = 0; i < NUM_OF_STORAGE; i++)
                        {
                            DIR *dir = opendir(storage_info[i].mount_path);
                            if(dir)
                            {
                                struct dirent *dir_items;
                                while((dir_items = readdir(dir)) != NULL)
                                {
                                    USB_LOG_INFO("%d dir item:%s\r\n",__LINE__,dir_items->d_name);
                                    if(_usb_mtp_hidden_file(dir_items->d_name)) 
                                    {
                                        USB_LOG_INFO("hidden file:%s\r\n",dir_items->d_name);
                                        continue;
                                    }
                                    objectcnt++;
                                }
                                closedir(dir);
                            }
                            else
                            {
                                USB_LOG_ERR("open dir err\r\n");
                            }
                        }
                    }
                    else
                    {
                        uint16_t idx = ((pack->parameter[0]>>16) & 0xffff)-1;
                        DIR *dir = opendir(storage_info[idx].mount_path);
                        if(dir)
                        {
                            struct dirent *dir_items;
                            while((dir_items = readdir(dir)) != NULL)
                            {
                                USB_LOG_INFO("%d dir item:%s\r\n",__LINE__,dir_items->d_name);
                                if(_usb_mtp_hidden_file(dir_items->d_name)) 
                                {
                                    USB_LOG_INFO("hidden file:%s\r\n",dir_items->d_name);
                                    continue;
                                }
                                objectcnt++;
                            }
                            closedir(dir);
                        }
                        else
                        {
                            USB_LOG_ERR("open dir err\r\n");
                        }
                    }
                }
                else if(pack->parameter[2] != 0)
                {
                    DIR *dir = opendir(item->str);
                    if(dir)
                    {
                        struct dirent *dir_items;
                        while((dir_items = readdir(dir)) != NULL)
                        {
                            objectcnt++;
                        }
                        closedir(dir);
                    }
                    else
                    {
                        USB_LOG_ERR("open dir err\r\n");
                    }
                }
                else
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_PARENT_OBJECT);
                }
                mtp_send_respond_ex(pack->transaction_id,MTP_RSP_OK,&objectcnt,1);
            }
            break;
            case MTP_THREAD_OP_GET_OBJECT_HANDLES:
            {
                uint32_t handle_cnt = 1;
                int max_cnt = (MTP_BUFFER_SIZE-4)/4;
                uint32_t *buf = (uint32_t *)mtp_buffer;
                mtp_packet_t *pack = (mtp_packet_t*)ep_out_buffer;
                handle_map_item_t *item;
                if(pack->parameter[2] != 0 && pack->parameter[2] != 0xffffffff)
                {
                    //check if folder is exist
                    item = get_list_item_by_handle(pack->parameter[2]);
                    if(item == NULL || !item->is_dir)
                    {
                        mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_HANDLE);
                        break;
                    }
                }
                if(pack->parameter[2] == 0xffffffff)
                {
                    //search root
                    if(pack->parameter[0] == 0xffffffff)
                    {
                        //search all storage
                        for(int i = 0; i < NUM_OF_STORAGE; i++)
                        {
                            uint16_t idx = ((pack->parameter[0]>>16) & 0xffff)-1;
                            DIR *dir = opendir(storage_info[i].mount_path);
                            if(dir)
                            {
                                struct dirent *dir_items;
                                while((dir_items = readdir(dir)) != NULL)
                                {
                                    USB_LOG_INFO("%d dir item:%s\r\n",__LINE__,dir_items->d_name);
                                    if(_usb_mtp_hidden_file(dir_items->d_name)) 
                                    {
                                        USB_LOG_INFO("hidden file:%s\r\n",dir_items->d_name);
                                        continue;
                                    }
                                    //USB_LOG_ERR("filename:%s,dir:0x%02X\r\n",dir_items->d_name,dir_items->d_type);
                                    uint32_t hd = map_add_item(0,storage_info[idx].mount_path,dir_items->d_name,(dir_items->d_type & DT_DIR),i+1);
                                    if(hd == 0) continue;
                                    buf[handle_cnt++] = hd;
                                    if(handle_cnt > max_cnt) break;
                                }
                                closedir(dir);
                            }
                            else
                            {
                                USB_LOG_ERR("open dir err\r\n");
                            }
                            if(handle_cnt > max_cnt)
                            {
                                USB_LOG_ERR("mtp_buffer out\r\n");
                                break;
                            }
                        }
                    }
                    else
                    {
                        uint16_t idx = ((pack->parameter[0]>>16) & 0xffff)-1;
                        DIR *dir = opendir(storage_info[idx].mount_path);
                        if(dir)
                        {
                            struct dirent *dir_items;
                            while((dir_items = readdir(dir)) != NULL)
                            {
                                USB_LOG_INFO("%d dir item:%s\r\n",__LINE__,dir_items->d_name);
                                if(_usb_mtp_hidden_file(dir_items->d_name)) 
                                {
                                    USB_LOG_INFO("hidden file:%s\r\n",dir_items->d_name);
                                    continue;
                                }
                                //USB_LOG_ERR("filename:%s,dir:0x%02X\r\n",dir_items->d_name,dir_items->d_type);
                                uint32_t hd = map_add_item(0,storage_info[idx].mount_path,dir_items->d_name,(dir_items->d_type & DT_DIR),idx+1);
                                if(hd == 0) continue;
                                buf[handle_cnt++] = hd;
                                if(handle_cnt > max_cnt)
                                {
                                    USB_LOG_ERR("mtp_buffer out\r\n");
                                    break;
                                }
                            }
                            closedir(dir);
                        }
                        else
                        {
                            USB_LOG_ERR("open dir err\r\n");
                        }
                    }
                }
                else if(pack->parameter[2] != 0)
                {
                    DIR *dir = opendir(item->str);
                    if(dir)
                    {
                        struct dirent *dir_items;
                        while((dir_items = readdir(dir)) != NULL)
                        {
                            //USB_LOG_ERR("filename:%s,dir:0x%02X\r\n",dir_items->d_name,dir_items->d_type);
                            uint32_t hd = map_add_item(pack->parameter[2],item->str,dir_items->d_name,(dir_items->d_type & DT_DIR),item->storage_id);
                            if(hd == 0) continue;
                            buf[handle_cnt++] = hd;
                            if(handle_cnt > max_cnt)
                            {
                                USB_LOG_ERR("mtp_buffer out\r\n");
                                break;
                            }
                        }
                        closedir(dir);
                    }
                    else
                    {
                        USB_LOG_ERR("open dir err\r\n");
                    }
                }
                else
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_PARENT_OBJECT);
                }
                buf[0] = handle_cnt-1;
                mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,handle_cnt*4);
            }
            break;
            case MTP_THREAD_OP_GET_OBJECT_INFO:
            {
                mtp_packet_t *pack = (mtp_packet_t*)ep_out_buffer;
                handle_map_item_t *item = get_list_item_by_handle(pack->parameter[0]);
                if(item == NULL)
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_HANDLE);
                    break;
                }
                mtp_objectinfo_dataset_t *obj_info = (mtp_objectinfo_dataset_t *)mtp_buffer;
                memset(obj_info,0x00,sizeof(mtp_objectinfo_dataset_t));
                obj_info->storage_id = (item->storage_id<<16) | 0x0001;
                if(item->is_dir)
                {
                    obj_info->object_format = MTP_OF_ASSOCIATION;
                    obj_info->association_type = 0x0001;
                }
                else
                {
                    obj_info->object_format = MTP_OF_UNDEFINED;
                }
                obj_info->parent_object = item->parent_handle;
                int strlen = Gb2312ToUtf16LE(get_file_name_from_path(item->str),obj_info->str);
                strlen += Gb2312ToUtf16LE("20250101T000000",&obj_info->str[strlen]);
                strlen += Gb2312ToUtf16LE("20250111T000000",&obj_info->str[strlen]);
                obj_info->str[strlen++] = 0x01;
                obj_info->str[strlen++] = 0x00;
                obj_info->str[strlen++] = 0x00;
                mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,sizeof(mtp_objectinfo_dataset_t) + strlen);
            }
            break;
            case MTP_THREAD_OP_GET_OBJECT_SIZE:
            {
                mtp_packet_t *pack = (mtp_packet_t*)ep_out_buffer;
                handle_map_item_t *item = get_list_item_by_handle(pack->parameter[0]);
                if(item == NULL)
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_HANDLE);
                    break;
                }
                if(item->is_dir)
                {
                    memset(mtp_buffer,0x00,8);
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,8);
                }
                else
                {
                    char *fullpath = get_full_path_by_item(item);
                    if(fullpath == NULL)
                    {
                        mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                        break;
                    }
                    struct stat st;
                    int ret = stat(fullpath,&st);
                    psram_free(fullpath);
                    if(ret)
                    {
                        memset(mtp_buffer,0x00,8);
                        mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,8);
                        USB_LOG_ERR("get file size fail:%d\r\n",ret);
                        break;
                    }
                    uint64_t filesize = st.st_size;
                    memcpy(mtp_buffer,&filesize,sizeof(uint64_t));
                    mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,sizeof(uint64_t));
                }
            }
            break;
            case MTP_THREAD_OP_GET_OBJECT_FILE_NAME:
            {
                mtp_packet_t *pack = (mtp_packet_t*)ep_out_buffer;
                handle_map_item_t *item = get_list_item_by_handle(pack->parameter[0]);
                if(item == NULL)
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_HANDLE);
                    break;
                }
                int strlen;
                //USB_LOG_ERR("file name:%s\r\n",item->str);
                if(item->is_dir)
                    strlen = Gb2312ToUtf16LE(get_file_name_from_path(item->str),mtp_buffer);
                else
                    strlen = Gb2312ToUtf16LE(item->str,mtp_buffer);
                mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,strlen);
            }
            break;
            case MTP_THREAD_OP_SET_OBJECT_PROP_VALUE:
            {
                mtp_packet_t *pack = (mtp_packet_t*)ep_out_buffer;
                if(mtp_parameter_backup[1] == MTP_OB_PROP_OBJ_FILE_NAME)
                {
                    handle_map_item_t *item = get_list_item_by_handle(mtp_parameter_backup[0]);
                    do
                    {
                        if(item == NULL) 
                        {
                            mtp_send_respond(pack->transaction_id,MTP_RSP_ACCESS_DENIED);
                            break;
                        }
                        Utf16LEToGb2312(pack->payload,mtp_buffer);
                        if(item->is_dir)
                        {
                            int pathlen = get_path_len(item->str)+1;
                            char *new_path = psram_malloc(pathlen + strlen((const char*)mtp_buffer) + 2);
                            if(new_path == NULL)
                            {
                                mtp_send_respond(pack->transaction_id,MTP_RSP_ACCESS_DENIED);
                                break;
                            }
                            memcpy(new_path,item->str,pathlen);
                            strcpy(&new_path[pathlen],(const char*)mtp_buffer);
                            int ret = rename(item->str,new_path);
                            if(ret)
                            {
                                psram_free(new_path);
                                mtp_send_respond(pack->transaction_id,MTP_RSP_ACCESS_DENIED);
                                break;
                            }
                            psram_free(item->str);
                            item->str = new_path;
                        }
                        else
                        {
                            char *path;
                            if(item->parent_handle)
                            {
                                handle_map_item_t *path_item = get_list_item_by_handle(item->parent_handle);
                                if(path_item == NULL || !path_item->is_dir)
                                {
                                    mtp_send_respond(pack->transaction_id,MTP_RSP_ACCESS_DENIED);
                                    break;
                                }
                                path = path_item->str;
                            }
                            else
                            {
                                path = (char*)storage_info[item->storage_id-1].mount_path;
                            }
                            char *oldpath = psram_malloc(strlen(path)+strlen(item->str)+2);
                            if(oldpath == NULL)
                            {
                                mtp_send_respond(pack->transaction_id,MTP_RSP_ACCESS_DENIED);
                                break;
                            }
                            char *newpath = psram_malloc(strlen(path)+strlen((const char*)mtp_buffer)+2);
                            if(newpath == NULL)
                            {
                                psram_free(oldpath);
                                mtp_send_respond(pack->transaction_id,MTP_RSP_ACCESS_DENIED);
                                break;
                            }
                            sprintf(oldpath,"%s/%s",path,item->str);
                            sprintf(newpath,"%s/%s",path,mtp_buffer);
                            int ret = rename(oldpath,newpath);
                            psram_free(oldpath);
                            psram_free(newpath);
                            if(ret)
                            {
                                mtp_send_respond(pack->transaction_id,MTP_RSP_ACCESS_DENIED);
                                break;
                            }
                            char *newfilename = psram_malloc(strlen((const char*)mtp_buffer)+1);
                            if(newfilename == NULL)
                            {
                                mtp_send_respond(pack->transaction_id,MTP_RSP_ACCESS_DENIED);
                                break;
                            }
                            strcpy(newfilename,(const char*)mtp_buffer);
                            psram_free(item->str);
                            item->str = newfilename;
                        }
                        mtp_send_respond(pack->transaction_id,MTP_RSP_OK);
                    }while(0);
                }
                else if(mtp_parameter_backup[1] == MTP_OB_PROP_NAME)
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_OK);
                }
                else
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_PROP_CODE);
                }
            }
            break;
            case MTP_THREAD_OP_SEND_OBJECT_INFO:
            {
                mtp_packet_t *pack = (mtp_packet_t*)ep_out_buffer;
                mtp_objectinfo_dataset_t *obj_dataset = (mtp_objectinfo_dataset_t *)pack->payload;

                Utf16LEToGb2312(obj_dataset->str,mtp_buffer);
                if(strlen((const char*)mtp_buffer) == 0)
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_DATASET);
                    break;
                }
                char *path;
                uint32_t parent_handle; 
                if(mtp_parameter_backup[1] != 0xffffffff)
                {
                    handle_map_item_t *item = get_list_item_by_handle(mtp_parameter_backup[1]);
                    path = item->str;
                    parent_handle = mtp_parameter_backup[1];
                }
                else
                {
                    int idx = ((mtp_parameter_backup[0]>>16)&0xffff)-1;
                    path = (char *)storage_info[idx].mount_path;
                    parent_handle = 0;
                }
                char *newpath = psram_malloc(strlen(path) + strlen((const char*)mtp_buffer) + 2);
                if(newpath == NULL)
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_ACCESS_DENIED);
                    break;
                }
                sprintf(newpath,"%s/%s",path,mtp_buffer);
                if(obj_dataset->object_format == 0x3001)
                {
                    int ret = mkdir(newpath,0);
                    if(ret)
                    {
                        psram_free(newpath);
                        mtp_send_respond(pack->transaction_id,MTP_RSP_ACCESS_DENIED);
                        break;
                    }
                    int idx = ((mtp_parameter_backup[0]>>16)&0xffff);
                    uint32_t hd = map_add_item(parent_handle,path,(const char*)mtp_buffer,1,idx);
                    if(hd == 0)
                    {
                        psram_free(newpath);
                        mtp_send_respond(pack->transaction_id,MTP_RSP_ACCESS_DENIED);
                        break;
                    }
                    last_sendinfo_handle = hd;
                    mtp_parameter_backup[2] = hd;
                    mtp_send_respond_ex(pack->transaction_id,MTP_RSP_OK,mtp_parameter_backup,3);
                }
                else
                {
                    int ret = mtp_sd_open_retry(newpath, O_CREAT);
                    if(ret < 0)
                    {
                        USB_LOG_ERR("mtp sd create GIVE UP\r\n");
                        psram_free(newpath);
                        mtp_send_respond(pack->transaction_id,MTP_RSP_INCOMPLETE_TRANSFER);
                        break;
                    }
                    close(ret);
                    int idx = ((mtp_parameter_backup[0]>>16)&0xffff);
                    uint32_t hd = map_add_item(parent_handle,path,(const char*)mtp_buffer,0,idx);
                    if(hd == 0)
                    {
                        psram_free(newpath);
                        mtp_send_respond(pack->transaction_id,MTP_RSP_ACCESS_DENIED);
                        break;
                    }
                    last_sendinfo_handle = hd;
                    mtp_parameter_backup[2] = hd;
                    mtp_send_respond_ex(pack->transaction_id,MTP_RSP_OK,mtp_parameter_backup,3);
                }
                psram_free(newpath);
            }
            break;
            case MTP_THREAD_OP_SEND_OBJECT:
            {
                if(open_fd < 0)
                {
                    handle_map_item_t *item = get_list_item_by_handle(last_sendinfo_handle);
                    if(item == NULL || item->is_dir)
                    {
                        mtp_send_respond(mtp_parameter_backup[0],MTP_RSP_ACCESS_DENIED);
                        break;
                    }
                    char *fullpath = get_full_path_by_item(item);
                    if(fullpath == NULL)
                    {
                        mtp_send_respond(mtp_parameter_backup[0],MTP_RSP_DEVICE_BUSY);
                        break;
                    }
                    open_fd = mtp_sd_open_retry(fullpath, O_CREAT|O_WRONLY);
                    psram_free(fullpath);
                    if(open_fd < 0)
                    {
                        USB_LOG_ERR("mtp sd open GIVE UP, drop file\r\n");
                        mtp_ep_recive_cnt = 0;
                        mtp_ep_recive_total_size = 0;
                        mtp_send_respond(mtp_parameter_backup[0],MTP_RSP_INCOMPLETE_TRANSFER);
                        break;
                    }
                }
                if(mtp_parameter_backup[2])
                {
                    uint8_t *wp = (uint8_t *)(mtp_parameter_backup[1]);
                    uint32_t remain = mtp_parameter_backup[2];
                    uint32_t io_fail = 0;
                    /* Wait for the SD card instead of dropping data. While the
                     * card is momentarily busy, write() fails; keep retrying (OUT
                     * stays un-armed so the host NAK-waits and the bar just
                     * pauses) until the write lands. A file copy must never
                     * truncate a file just because the card is briefly busy. */
                    while(remain)
                    {
                        int ret = write(open_fd, wp, remain);
                        if(ret > 0)
                        {
                            wp += ret;
                            remain -= (uint32_t)ret;
                            io_fail = 0;
                            continue;
                        }
                        io_fail++;
                        if((io_fail % 100) == 1)
                            USB_LOG_ERR("mtp sd write busy ret=%d remain=%d try=%d\r\n", ret, remain, io_fail);
                        if(io_fail > MTP_SD_RETRY_MAX)
                            break;
                        usb_osal_msleep(MTP_SD_RETRY_MS);
                    }
                    if(remain)
                    {
                        /* Card never freed within the (large) budget -- treat as a
                         * genuine failure (card pulled / FS error), not transient.
                         * Delete the half-written file and fail LOUDLY so the host
                         * surfaces an error instead of silently keeping a
                         * truncated file (silent data loss is the worst outcome
                         * for a backup copy). */
                        USB_LOG_ERR("mtp sd write GIVE UP remain=%d, drop file\r\n", remain);
                        close(open_fd);
                        open_fd = -1;
                        {
                            handle_map_item_t *fi = get_list_item_by_handle(last_sendinfo_handle);
                            if(fi)
                            {
                                char *fp = get_full_path_by_item(fi);
                                if(fp){ unlink(fp); psram_free(fp); }
                            }
                        }
                        mtp_ep_recive_cnt = 0;
                        mtp_ep_recive_total_size = 0;
                        mtp_send_respond(mtp_parameter_backup[0],MTP_RSP_INCOMPLETE_TRANSFER);
                        break;
                    }
                    if(mtp_parameter_backup[3])
                    {
                        close(open_fd);
                        open_fd = -1;
                        mtp_send_respond(mtp_parameter_backup[0],MTP_RSP_OK);
                    }
                    else
                    {
                        usbd_ep_start_read(mtp_ep_data[MTP_OUT_EP_IDX].ep_addr, ep_out_buffer, mtp_next_rx_len());
                    }
                }
            }
            break;
            case MTP_THREAD_OP_DELETE_OBJECT:
            {
                mtp_packet_t *pack = (mtp_packet_t*)ep_out_buffer;
                handle_map_item_t *item = get_list_item_by_handle(pack->parameter[0]);
                if(item == NULL)
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_HANDLE);
                    break;
                }
                if(item->is_dir)
                {
                    int ret = mtp_rmdir_recursive(item->str);
                    remove_object_recursive_by_handle(item->handle);
                    mtp_send_respond(pack->transaction_id, ret == 0 ? MTP_RSP_OK : MTP_RSP_ACCESS_DENIED);
                }
                else
                {
                    char *fullpath = get_full_path_by_item(item);
                    if(fullpath == NULL)
                    {
                        mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                        break;
                    }
                    unlink(fullpath);
                    psram_free(fullpath);
                    remove_object_recursive_by_handle(item->handle);
                    mtp_send_respond(pack->transaction_id,MTP_RSP_OK);
                }
            }
            break;
            case MTP_THREAD_OP_GET_OBJECT:
            {
                if(open_fd < 0)
                {
                    mtp_packet_t *pack = (mtp_packet_t*)ep_out_buffer;
                    handle_map_item_t *item = get_list_item_by_handle(pack->parameter[0]);
                    if(item == NULL || item->is_dir)
                    {
                        mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_HANDLE);
                        break;
                    }
                    char *fullpath = get_full_path_by_item(item);
                    if(fullpath == NULL)
                    {
                        mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                        break;
                    }

                    struct stat st;
                    int ret = stat(fullpath,&st);
                    if(ret)
                    {
                        psram_free(fullpath);
                        mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                        USB_LOG_ERR("get file size fail:%d\r\n",ret);
                        break;
                    }
                    mtp_parameter_backup[0] = pack->transaction_id;
                    uint32_t filesize = (uint32_t)st.st_size;

                    mtp_packet_t *sendpack = (mtp_packet_t *)ep_in_buffer;
                    sendpack->container_type = MTP_CONT_TYPE_DATA;
                    sendpack->transaction_id = mtp_parameter_backup[0];
                    if(pack->code == MTP_OP_GET_PARTIAL_OBJECT)
                    {
                        if(pack->parameter[2] == 0xffffffff && pack->parameter[1] >= filesize)
                        {
                            psram_free(fullpath);
                            mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_PARAMETER);
                            break;
                        }
                        else if(!pack->parameter[2] || (pack->parameter[2] != 0xffffffff && pack->parameter[1] + pack->parameter[2] >= filesize))
                        {
                            psram_free(fullpath);
                            mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_PARAMETER);
                            break;
                        }
                        sendpack->code = MTP_OP_GET_PARTIAL_OBJECT;
                        sendpack->container_length = 12 + filesize;
                        current_sending_total_size = pack->parameter[1] + pack->parameter[2];
                        current_sending_idx = pack->parameter[1];
                    }
                    else
                    {
                        sendpack->code = MTP_OP_GET_OBJECT;
                        sendpack->container_length = 12 + filesize;
                        current_sending_total_size = filesize;
                        current_sending_idx = 0;
                    }
                    
                    if(filesize)
                    {
                        open_fd = open(fullpath,O_CREAT);
                        psram_free(fullpath);
                        if(open_fd < 0)
                        {
                            psram_free(fullpath);
                            mtp_send_respond(mtp_parameter_backup[0],MTP_RSP_DEVICE_BUSY);
                            break;
                        }
                        if(current_sending_idx)
                        {
                            int ret = lseek(open_fd,current_sending_idx,SEEK_SET);
                            if(ret < 0)
                            {
                                close(open_fd);
                                open_fd = -1;
                                mtp_send_respond(mtp_parameter_backup[0],MTP_RSP_DEVICE_BUSY);
                                break;
                            }
                        }
                        int ret = read(open_fd,&ep_in_buffer[12],MAX_PACKET_SIZE-12);
                        if(ret <= 0)
                        {
                            close(open_fd);
                            open_fd = -1;
                            mtp_send_respond(mtp_parameter_backup[0],MTP_RSP_DEVICE_BUSY);
                            break;
                        }
                        mtp_state = MTP_STATE_SEND_OBJ_DATA;
                        usbd_ep_start_write(mtp_ep_data[MTP_IN_EP_IDX].ep_addr,ep_in_buffer, 12+ret);
                    }
                    else
                    {
                        psram_free(fullpath);
                        mtp_state = MTP_STATE_SEND_OBJ_DATA;
                        usbd_ep_start_write(mtp_ep_data[MTP_IN_EP_IDX].ep_addr,ep_in_buffer, 12);
                    }
                }
                else
                {
                    int ret = read(open_fd,ep_in_buffer,MAX_PACKET_SIZE);
                    if(ret > 0)
                    {
                        current_sending_idx += ret;
                        usbd_ep_start_write(mtp_ep_data[MTP_IN_EP_IDX].ep_addr,ep_in_buffer, ret);
                    }
                    else
                    {
                        close(open_fd);
                        open_fd = -1;
                        if(current_sending_idx >= current_sending_total_size)
                        {
                            mtp_send_respond(mtp_parameter_backup[0],MTP_RSP_OK);
                        }
                        else
                        {
                            mtp_send_respond(mtp_parameter_backup[0],MTP_RSP_INCOMPLETE_TRANSFER);
                        }
                    }
                }
            }
            break;
            case MTP_THREAD_OP_GET_OBJECT_PROPLIST:
            {
                mtp_packet_t *pack = (mtp_packet_t*)ep_out_buffer;
                handle_map_item_t *item = get_list_item_by_handle(pack->parameter[0]);
                int idx = 0;
                if(pack->parameter[3] == 0x00000004)
                {
                    uint32_t temp = 4;
                    memcpy(&mtp_buffer[idx],&temp,4);
                    idx+=4;

                    memcpy(&mtp_buffer[idx],&pack->parameter[0],4);
                    idx+=4;
                    mtp_buffer[idx++] = MTP_OB_PROP_OBJECT_FORMAT&0xff;
                    mtp_buffer[idx++] = (MTP_OB_PROP_OBJECT_FORMAT>>8)&0xff;
                    mtp_buffer[idx++] = MTP_DATATYPE_UINT16&0xff;
                    mtp_buffer[idx++] = (MTP_DATATYPE_UINT16>>8)&0xff;
                    if(item->is_dir)
                    {
                        mtp_buffer[idx++] = MTP_OF_ASSOCIATION&0xff;
                        mtp_buffer[idx++] = (MTP_OF_ASSOCIATION>>8)&0xff;
                    }
                    else
                    {
                        mtp_buffer[idx++] = MTP_OF_UNDEFINED&0xff;
                        mtp_buffer[idx++] = (MTP_OF_UNDEFINED>>8)&0xff;
                    }

                    memcpy(&mtp_buffer[idx],&pack->parameter[0],4);
                    idx+=4;
                    mtp_buffer[idx++] = MTP_OB_PROP_OBJECT_SIZE&0xff;
                    mtp_buffer[idx++] = (MTP_OB_PROP_OBJECT_SIZE>>8)&0xff;
                    mtp_buffer[idx++] = MTP_DATATYPE_UINT64&0xff;
                    mtp_buffer[idx++] = (MTP_DATATYPE_UINT64>>8)&0xff;
                    if(item->is_dir)
                    {
                        memset(&mtp_buffer[idx],0x00,8);
                        idx += 8;
                    }
                    else
                    {
                        char *fullpath;
                        if(item->parent_handle)
                        {
                            handle_map_item_t *path_item = get_list_item_by_handle(item->parent_handle);
                            fullpath = psram_malloc(strlen(path_item->str) + strlen(item->str) + 2);
                            if(fullpath == NULL)
                            {
                                mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                                break;
                            }
                            sprintf(fullpath,"%s/%s",path_item->str,item->str);
                        }
                        else
                        {
                            fullpath = psram_malloc(strlen(storage_info[item->storage_id-1].mount_path) + strlen(item->str) + 2);
                            if(fullpath == NULL)
                            {
                                mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                                break;
                            }
                            sprintf(fullpath,"%s/%s",storage_info[item->storage_id-1].mount_path,item->str);
                        }
                        struct stat st;
                        int ret = stat(fullpath,&st);
                        psram_free(fullpath);
                        if(ret)
                        {
                            mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                            USB_LOG_ERR("get file size fail:%d\r\n",ret);
                            break;
                        }
                        uint64_t filesize = st.st_size;
                        memcpy(&mtp_buffer[idx],&filesize,sizeof(filesize));
                        idx += 8;
                    }

                    memcpy(&mtp_buffer[idx],&pack->parameter[0],4);
                    idx+=4;
                    mtp_buffer[idx++] = MTP_OB_PROP_OBJ_FILE_NAME&0xff;
                    mtp_buffer[idx++] = (MTP_OB_PROP_OBJ_FILE_NAME>>8)&0xff;
                    mtp_buffer[idx++] = MTP_DATATYPE_STR&0xff;
                    mtp_buffer[idx++] = (MTP_DATATYPE_STR>>8)&0xff;
                    if(item->is_dir)
                        idx += Gb2312ToUtf16LE(get_file_name_from_path(item->str),&mtp_buffer[idx]);
                    else
                        idx += Gb2312ToUtf16LE(item->str,&mtp_buffer[idx]);

                    memcpy(&mtp_buffer[idx],&pack->parameter[0],4);
                    idx+=4;
                    mtp_buffer[idx++] = MTP_OB_PROP_PERS_UNIQ_OBJ_IDEN&0xff;
                    mtp_buffer[idx++] = (MTP_OB_PROP_PERS_UNIQ_OBJ_IDEN>>8)&0xff;
                    mtp_buffer[idx++] = MTP_DATATYPE_UINT128&0xff;
                    mtp_buffer[idx++] = (MTP_DATATYPE_UINT128>>8)&0xff;
                    memset(&mtp_buffer[idx],0x00,16);
                    memcpy(&mtp_buffer[idx],&pack->parameter[0],4);
                    idx += 16;
                }
                else
                {
                    uint32_t temp = 9;
                    memcpy(&mtp_buffer[idx],&temp,4);
                    idx+=4;

                    memcpy(&mtp_buffer[idx],&pack->parameter[0],4);
                    idx+=4;
                    mtp_buffer[idx++] = MTP_OB_PROP_STORAGE_ID&0xff;
                    mtp_buffer[idx++] = (MTP_OB_PROP_STORAGE_ID>>8)&0xff;
                    mtp_buffer[idx++] = MTP_DATATYPE_UINT32&0xff;
                    mtp_buffer[idx++] = (MTP_DATATYPE_UINT32>>8)&0xff;
                    temp = (item->storage_id<<16)|0x0001;
                    memcpy(&mtp_buffer[idx],&temp,4);
                    idx+=4;

                    memcpy(&mtp_buffer[idx],&pack->parameter[0],4);
                    idx+=4;
                    mtp_buffer[idx++] = MTP_OB_PROP_PROTECTION_STATUS&0xff;
                    mtp_buffer[idx++] = (MTP_OB_PROP_PROTECTION_STATUS>>8)&0xff;
                    mtp_buffer[idx++] = MTP_DATATYPE_UINT16&0xff;
                    mtp_buffer[idx++] = (MTP_DATATYPE_UINT16>>8)&0xff;
                    mtp_buffer[idx++] = 0x00;
                    mtp_buffer[idx++] = 0x00;

                    memcpy(&mtp_buffer[idx],&pack->parameter[0],4);
                    idx+=4;
                    mtp_buffer[idx++] = MTP_OB_PROP_ASSOC_TYPE&0xff;
                    mtp_buffer[idx++] = (MTP_OB_PROP_ASSOC_TYPE>>8)&0xff;
                    mtp_buffer[idx++] = MTP_DATATYPE_UINT16&0xff;
                    mtp_buffer[idx++] = (MTP_DATATYPE_UINT16>>8)&0xff;
                    if(item->is_dir)
                    {
                        mtp_buffer[idx++] = 0x01;
                        mtp_buffer[idx++] = 0x00;
                    }
                    else
                    {
                        mtp_buffer[idx++] = 0x00;
                        mtp_buffer[idx++] = 0x00;
                    }

                    memcpy(&mtp_buffer[idx],&pack->parameter[0],4);
                    idx+=4;
                    mtp_buffer[idx++] = MTP_OB_PROP_NON_CONSUMABLE&0xff;
                    mtp_buffer[idx++] = (MTP_OB_PROP_NON_CONSUMABLE>>8)&0xff;
                    mtp_buffer[idx++] = MTP_DATATYPE_UINT8&0xff;
                    mtp_buffer[idx++] = (MTP_DATATYPE_UINT8>>8)&0xff;
                    mtp_buffer[idx++] = 0x00;

                    memcpy(&mtp_buffer[idx],&pack->parameter[0],4);
                    idx+=4;
                    mtp_buffer[idx++] = MTP_OB_PROP_DATE_CREATED&0xff;
                    mtp_buffer[idx++] = (MTP_OB_PROP_DATE_CREATED>>8)&0xff;
                    mtp_buffer[idx++] = MTP_DATATYPE_STR&0xff;
                    mtp_buffer[idx++] = (MTP_DATATYPE_STR>>8)&0xff;
                    idx += Gb2312ToUtf16LE("20250101T000000",&mtp_buffer[idx]);

                    memcpy(&mtp_buffer[idx],&pack->parameter[0],4);
                    idx+=4;
                    mtp_buffer[idx++] = MTP_OB_PROP_DATE_MODIFIED&0xff;
                    mtp_buffer[idx++] = (MTP_OB_PROP_DATE_MODIFIED>>8)&0xff;
                    mtp_buffer[idx++] = MTP_DATATYPE_STR&0xff;
                    mtp_buffer[idx++] = (MTP_DATATYPE_STR>>8)&0xff;
                    idx += Gb2312ToUtf16LE("20250101T000000",&mtp_buffer[idx]);

                    memcpy(&mtp_buffer[idx],&pack->parameter[0],4);
                    idx+=4;
                    mtp_buffer[idx++] = MTP_OB_PROP_PARENT_OBJECT&0xff;
                    mtp_buffer[idx++] = (MTP_OB_PROP_PARENT_OBJECT>>8)&0xff;
                    mtp_buffer[idx++] = MTP_DATATYPE_UINT32&0xff;
                    mtp_buffer[idx++] = (MTP_DATATYPE_UINT32>>8)&0xff;
                    memcpy(&mtp_buffer[idx],&item->parent_handle,4);
                    idx += 4;

                    memcpy(&mtp_buffer[idx],&pack->parameter[0],4);
                    idx+=4;
                    mtp_buffer[idx++] = MTP_OB_PROP_NAME&0xff;
                    mtp_buffer[idx++] = (MTP_OB_PROP_NAME>>8)&0xff;
                    mtp_buffer[idx++] = MTP_DATATYPE_STR&0xff;
                    mtp_buffer[idx++] = (MTP_DATATYPE_STR>>8)&0xff;
                    if(item->is_dir)
                        idx += Gb2312ToUtf16LE(get_file_name_from_path(item->str),&mtp_buffer[idx]);
                    else
                        idx += Gb2312ToUtf16LE(item->str,&mtp_buffer[idx]);

                    memcpy(&mtp_buffer[idx],&pack->parameter[0],4);
                    idx+=4;
                    mtp_buffer[idx++] = MTP_OB_PROP_DRM_STATUS&0xff;
                    mtp_buffer[idx++] = (MTP_OB_PROP_DRM_STATUS>>8)&0xff;
                    mtp_buffer[idx++] = MTP_DATATYPE_UINT16&0xff;
                    mtp_buffer[idx++] = (MTP_DATATYPE_UINT16>>8)&0xff;
                    mtp_buffer[idx++] = 0x00;
                    mtp_buffer[idx++] = 0x00;
                }
                mtp_send_data(pack->transaction_id,pack->code,mtp_buffer,idx);
            }
            break;
            case MTP_THREAD_OP_FORMAT_STORAGE:
            {
                mtp_packet_t *pack = (mtp_packet_t*)ep_out_buffer;
                int idx = ((pack->parameter[0]>>16)&0xffff)-1;
                int ret = glass_fs_umount((char*)storage_info[idx].mount_path);
                if(ret != BK_OK)
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                    break;
                }
                ret = glass_fs_format((char*)storage_info[idx].mount_path);
                if(ret != BK_OK)
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                    break;
                }
                ret = glass_fs_mount((char*)storage_info[idx].mount_path);
                if(ret != BK_OK)
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                    break;
                }
                clear_all_list_by_storage_id(idx+1);
                mtp_send_respond(pack->transaction_id,MTP_RSP_OK);
            }
            break;
            case MTP_THREAD_OP_MOVE_OBJECT:
            {
                mtp_packet_t *pack = (mtp_packet_t*)ep_out_buffer;
                handle_map_item_t *item = get_list_item_by_handle(pack->parameter[0]);
                if(item == NULL)
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_HANDLE);
                    break;
                }
                if(((pack->parameter[1]>>16)&0xffff) != item->storage_id)
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_STORAGE_ID);
                    break;
                }
                char *temppath;
                uint32_t newhandle;
                if(pack->parameter[2])
                {
                    handle_map_item_t *path = get_list_item_by_handle(pack->parameter[2]);
                    if(path == NULL || !path->is_dir)
                    {
                        mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_PARENT_OBJECT);
                        break;
                    }
                    temppath = path->str;
                    newhandle = path->handle;
                }
                else
                {
                    temppath = (char*)storage_info[item->storage_id-1].mount_path;
                    newhandle = 0;
                }
                if(item->is_dir)
                {
                    const char *dir_name = get_file_name_from_path(item->str);
                    char *newpath = psram_malloc(strlen(temppath)+strlen(dir_name)+2);
                    if(newpath == NULL)
                    {
                        mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                        break;
                    }
                    sprintf(newpath,"%s/%s",temppath,dir_name);
                    int ret = rename(item->str,newpath);
                    if(ret)
                    {
                        psram_free(newpath);
                        mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                        break;
                    }
                    psram_free(item->str);
                    item->str = newpath;
                    item->parent_handle = newhandle;
                }
                else
                {
                    char *oldpath = get_full_path_by_item(item);
                    if(oldpath == NULL)
                    {
                        mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                        break;
                    }

                    char *newpath = psram_malloc(strlen(temppath)+strlen(item->str)+2);
                    if(newpath == NULL)
                    {
                        psram_free(oldpath);
                        mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                        break;
                    }
                    sprintf(newpath,"%s/%s",temppath,item->str);
                    int ret = rename(oldpath,newpath);
                    psram_free(oldpath);
                    psram_free(newpath);
                    if(ret)
                    {
                        mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                        break;
                    }
                    item->parent_handle = newhandle;
                }
                mtp_send_respond(pack->transaction_id,MTP_RSP_OK);
            }
            break;
            case MTP_THREAD_OP_COPY_OBJECT:
            {
                mtp_packet_t *pack = (mtp_packet_t*)ep_out_buffer;
                handle_map_item_t *item = get_list_item_by_handle(pack->parameter[0]);
                if(item == NULL)
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_HANDLE);
                    break;
                }
                char *temppath;
                uint32_t newhandle;
                if(pack->parameter[2])
                {
                    handle_map_item_t *path = get_list_item_by_handle(pack->parameter[2]);
                    if(path == NULL || !path->is_dir)
                    {
                        mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_PARENT_OBJECT);
                        break;
                    }
                    temppath = path->str;
                    newhandle = path->handle;
                }
                else
                {
                    temppath = (char*)storage_info[item->storage_id-1].mount_path;
                    newhandle = 0;
                }
                if(item->is_dir)
                {
                    mtp_send_respond(pack->transaction_id,MTP_RSP_INVALID_OBJECT_HANDLE);
                    break;
                }
                else
                {
                    char *oldpath = get_full_path_by_item(item);
                    if(oldpath == NULL)
                    {
                        mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                        break;
                    }

                    char *newpath = psram_malloc(strlen(temppath)+strlen(item->str)+2);
                    if(newpath == NULL)
                    {
                        psram_free(oldpath);
                        mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                        break;
                    }
                    sprintf(newpath,"%s/%s",temppath,item->str);
                    int wfd = open(newpath,O_WRONLY|O_TRUNC);
                    if(wfd < 0)
                    {
                        psram_free(oldpath);
                        psram_free(newpath);
                        mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                        break;
                    }
                    int rfd = open(oldpath,O_RDONLY);
                    if(rfd < 0)
                    {
                        psram_free(oldpath);
                        psram_free(newpath);
                        close(wfd);
                        mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                        break;
                    }
                    psram_free(oldpath);
                    uint8_t *tempbuffer = psram_malloc(MTP_COPY_BUFFER_SIZE);
                    if(tempbuffer == NULL)
                    {
                        psram_free(newpath);
                        close(wfd);
                        close(rfd);
                        mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                        break;
                    }
                    int rd = 0,wr = 0;
                    while((rd = read(rfd,tempbuffer,MTP_COPY_BUFFER_SIZE))>0)
                    {
                        wr = write(wfd,tempbuffer,rd);
                        if(wr < 0)
                        {
                            break;
                        }
                        if(mtp_thread_op == MTP_THREAD_OP_CANCEL_REQUEST)
                        {
                            wr = -1;
                            break;
                        }
                    }
                    close(wfd);
                    close(rfd);
                    psram_free(tempbuffer);
                    if(rd < 0 || wr < 0)
                    {
                        unlink(newpath);
                        psram_free(newpath);
                        mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                        break;
                    }
                    psram_free(newpath);
                    uint32_t handle = map_add_item(newhandle,temppath,item->str,0,(pack->parameter[1]>>16)&0xffff);
                    if(!handle)
                    {
                        mtp_send_respond(pack->transaction_id,MTP_RSP_DEVICE_BUSY);
                        break;
                    }
                    mtp_send_respond_ex(pack->transaction_id,MTP_RSP_OK,&handle,1);
                }
            }
            break;
            case MTP_THREAD_OP_RESET:
            {
                mtp_env_reset();
                if(open_fd >= 0)
                {
                    close(open_fd);
                    open_fd = -1;
                }
                clear_all_list();
            }
            break;
            case MTP_THREAD_OP_CANCEL_REQUEST:
            {
                USB_LOG_INFO("%d\r\n",mtp_state);
                if(mtp_state == MTP_STATE_RECIVE_OBJ_DATA)
                {
                    mtp_ep_recive_total_size = 0;
                    mtp_ep_recive_cnt = 0;
                    if(open_fd >= 0)
                    {
                        close(open_fd);
                        open_fd = -1;
                        handle_map_item_t *item = get_list_item_by_handle(last_sendinfo_handle);
                        if(item)
                        {
                            char *fullpath = get_full_path_by_item(item);
                            if(fullpath)
                            {
                                unlink(fullpath);
                                psram_free(fullpath);
                                remove_object_recursive_by_handle(item->handle);
                            }
                        }
                    }
                    usbd_ep_set_stall(mtp_ep_data[MTP_OUT_EP_IDX].ep_addr);
                    usbd_ep_start_read(mtp_ep_data[MTP_OUT_EP_IDX].ep_addr, ep_out_buffer, MAX_PACKET_SIZE);
                    mtp_state = MTP_STATE_IDLE;
                }
                else if(mtp_state == MTP_STATE_SEND_OBJ_DATA)
                {
                    mtp_ep_recive_total_size = 0;
                    mtp_ep_recive_cnt = 0;
                    if(open_fd >= 0)
                    {
                        close(open_fd);
                        open_fd = -1;
                    }
                    usbd_ep_start_read(mtp_ep_data[MTP_OUT_EP_IDX].ep_addr, ep_out_buffer, MAX_PACKET_SIZE);
                    mtp_state = MTP_STATE_IDLE;
                }
                mtp_thread_op = MTP_THREAD_OP_NONE;
            }
            break;
            default:
            break;
        }
        #endif
        s_mtp_busy = 0;
    }
    #if CONFIG_VFS
    if(open_fd >= 0)
    {
        close(open_fd);
        open_fd = -1;
    }
    #endif
    clear_all_list();
    /* Hand the queue/thread teardown back to usb_mtp_deinit(): it is blocked on
     * mtp_exit_sem and will delete mtp_op_q + clear the handles only after we
     * signal here. Do NOT touch mtp_op_q/mtp_thread from this context anymore --
     * that async free was the source of the repeated start/stop crash. */
    if (mtp_exit_sem)
        usb_osal_sem_give(mtp_exit_sem);
    /* self-delete: v1.6 OSAL usb_osal_thread_delete(NULL) wraps to
     * rtos_delete_thread(&handle) with handle==NULL, which asserts in the
     * RTOS. Use rtos_delete_thread(NULL) for the proper self-delete path. */
    rtos_delete_thread(NULL);
}

struct usbd_interface *usbd_mtp_init_intf(struct usbd_interface *intf, const uint8_t in_ep,const uint8_t out_ep,const uint8_t int_ep)
{
    intf->class_interface_handler = mtp_class_interface_request_handler;
    intf->class_endpoint_handler = NULL;
    intf->vendor_handler = NULL;
    intf->notify_handler = mtp_notify_handler;

    mtp_ep_data[MTP_IN_EP_IDX].ep_addr = in_ep;
    mtp_ep_data[MTP_IN_EP_IDX].ep_cb = mtp_bulk_in;
    mtp_ep_data[MTP_OUT_EP_IDX].ep_addr = out_ep;
    mtp_ep_data[MTP_OUT_EP_IDX].ep_cb = mtp_bulk_out;
    mtp_ep_data[MTP_INT_EP_IDX].ep_addr = int_ep;
    mtp_ep_data[MTP_INT_EP_IDX].ep_cb = mtp_int_in;

    usbd_add_endpoint(MTP_BUSID, &mtp_ep_data[MTP_IN_EP_IDX]);
    usbd_add_endpoint(MTP_BUSID, &mtp_ep_data[MTP_OUT_EP_IDX]);
    usbd_add_endpoint(MTP_BUSID, &mtp_ep_data[MTP_INT_EP_IDX]);

    if(mtp_thread == NULL)
    {
        #if CONFIG_VFS
        path_list = NULL;
        file_list = NULL;
        next_handle = 1;
        open_fd = -1;
        #endif
        mtp_should_exit = 0;
        mtp_exit_sem = usb_osal_sem_create(0);
        mtp_op_q = usb_osal_mq_create(8);
        mtp_thread = usb_osal_thread_create("usbd_mtp", 2048, BEKEN_DEFAULT_WORKER_PRIORITY, usbd_mtp_thread, NULL);
        if (mtp_thread == NULL) {
            USB_LOG_ERR("no enough memory to alloc mtp thread\r\n");
            return NULL;
        }
    }

    return intf;
}

static int _usb_mtp_hidden_file(const char *path)
{
    if(!s_hidden_file_enable)
    {
        return 0;
    }
    if(s_hidden_file_list == NULL || s_hidden_file_list_count == 0)
    {
        return 0;
    }
    for(uint32_t i = 0;i<s_hidden_file_list_count;i++)
    {
        if(s_hidden_file_list[i] == NULL)
        {
            continue;
        }
        if(strstr(path,s_hidden_file_list[i]) != NULL)
        {
            return 1;
        }
    }
    return 0;
}

void usb_mtp_set_hidden_file_enable(bool enable)
{
    s_hidden_file_enable = enable;
}

void usb_mtp_cfg_hidden_file_list(const char ** hidden_file_list,uint32_t count)
{
    s_hidden_file_list = hidden_file_list;
    s_hidden_file_list_count = count;
}

#if CONFIG_VFS
/* The MTP engine browses its storage through the bk_vfs POSIX layer, so make
 * sure the SD card FATFS is mounted at VFS_SD_0_PATITION_0 before bring-up.
 * Done here (rather than in the app) so the engine owns its storage and the
 * app file does not have to pull bk_vfs headers (which clash with ff.h's DIR). */
static void mtp_mount_storage(void)
{
    struct bk_fatfs_partition partition;
    int ret;

    partition.part_type = FATFS_DEVICE;
    partition.part_dev.device_name = FATFS_DEV_SDCARD;
    partition.mount_path = VFS_SD_0_PATITION_0;
    ret = mount("SOURCE_NONE", partition.mount_path, "fatfs", 0, &partition);
    if (ret != 0) {
        USB_LOG_ERR("[mtp] mount %s failed:%d (SD inserted/formatted?)\r\n",
                    VFS_SD_0_PATITION_0, ret);
    } else {
        USB_LOG_INFO("[mtp] mounted SD card at %s\r\n", VFS_SD_0_PATITION_0);
    }
}
#endif

int usb_mtp_init(void)
{
    if(s_mtp_init) return BK_OK;
    int ret = BK_OK;
#if CONFIG_VFS
    mtp_mount_storage();
#endif
    if(usb_pm == 0)
    {
        usb_pm = malloc_pm_lock();
    }
    ep_out_buffer = os_malloc(MTP_RX_CHUNK);
    if(ep_out_buffer == NULL)
    {
        USB_LOG_ERR("%s malloc ep_out_buffer fail", __func__);
        return BK_FAIL;
    }
    ep_in_buffer = os_malloc(MAX_PACKET_SIZE);
    if(ep_in_buffer == NULL)
    {
        USB_LOG_ERR("%s malloc ep_in_buffer fail", __func__);
        os_free(ep_out_buffer);
        return BK_FAIL;
    }
    mtp_buffer = os_malloc(MTP_BUFFER_SIZE);
    if(mtp_buffer == NULL)
    {
        USB_LOG_ERR("%s malloc mtp_buffer fail", __func__);
        os_free(ep_out_buffer);
        os_free(ep_in_buffer);
        return BK_FAIL;
    }
    pm_lock(usb_pm);
    /* Power the USB analog PHY + clock + OTG (B-device) before usbd_initialize,
     * exactly as bk_usb_open(USB_DEVICE_MODE) does for the legacy path. Keep USB
     * out of low-power sleep first (host path votes the same module). */
    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_USB_1, 0, 0);
    bk_analog_layer_usb_sys_related_ops(MTP_USB_DEVICE_MODE, true);
    usbd_desc_register(MTP_BUSID, &mtp_v16_descriptor);
    usbd_add_interface(MTP_BUSID, usbd_mtp_init_intf(&intf0, MTP_IN_EP, MTP_OUT_EP, MTP_INT_EP));
    ret = usbd_initialize(MTP_BUSID, SOC_USB_HS_BASE, mtp_v16_usbd_event_handler);
    if(ret != BK_OK) 
    {
        return ret;
    }
    mtp_env_reset();
    s_mtp_init = 1;

    return ret;

}

int usb_mtp_deinit(void)
{
    if(!s_mtp_init) return BK_OK;
    int ret = BK_OK;

    /* 1. Retire the worker thread SYNCHRONOUSLY. Signal EXIT, wake it, then block
     *    on mtp_exit_sem until it has actually returned. The USB device is still
     *    live here, so a worker that is mid-transfer finishes cleanly before it
     *    checks mtp_thread_op and breaks. */
    if(mtp_thread)
    {
        mtp_should_exit = 1;
        mtp_thread_op = MTP_THREAD_EXIT;
        mtp_wake_worker();
        if(mtp_exit_sem)
            usb_osal_sem_take(mtp_exit_sem, USB_OSAL_WAITING_FOREVER);
    }

    /* 2. Tear the USB device controller down. After usbd_deinitialize() the USB
     *    HS ISR is unregistered, so nothing can post to the worker queue behind
     *    our back once we start deleting the handles below. */
    if((ret = usbd_deinitialize(MTP_BUSID)) != BK_OK)
    {
        pm_unlock(usb_pm);
        return ret;
    }

    /* 3. Worker is gone and USB is quiesced: safe to delete the queue/sem and
     *    clear every handle. Next usb_mtp_init() will recreate a fresh set. */
    if(mtp_exit_sem)
    {
        usb_osal_sem_delete(mtp_exit_sem);
        mtp_exit_sem = NULL;
    }
    if(mtp_op_q)
    {
        usb_osal_mq_delete(mtp_op_q);
        mtp_op_q = NULL;
    }
    mtp_thread = NULL;

    os_free(ep_out_buffer);
    os_free(ep_in_buffer);
    os_free(mtp_buffer);
    ep_out_buffer = NULL;
    ep_in_buffer = NULL;
    mtp_buffer = NULL;
    /* Power the USB analog PHY back down and release the no-sleep vote. */
    bk_analog_layer_usb_sys_related_ops(MTP_USB_DEVICE_MODE, false);
    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_USB_1, 1, 0);
    pm_unlock(usb_pm);
    s_mtp_init = 0;
    return ret;
}
