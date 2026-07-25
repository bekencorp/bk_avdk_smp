#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/shell_task.h>
#include <components/bk_frame_buffer.h>
#include <stdint.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"

#if CONFIG_FATFS
#include "ff.h"
#endif

#if CONFIG_CLI
#include "cli.h"
#endif

#if (CONFIG_USB_HOST && CONFIG_USBH_MSC)
#include <components/usb.h>
#include <components/usb_types.h>
#include <components/cherryusb/usbh_core.h>
#include <components/cherryusb/usbh_msc.h>
#endif

extern void sys_ana_usb_phy_op(uint8_t en);
extern int msc_storage_init(void);
extern int msc_storage_deinit(void);

static beken_thread_t s_msc_init_thread = NULL;

#define USB_MSC_INIT_TASK_PRIORITY    BEKEN_DEFAULT_WORKER_PRIORITY
#define USB_MSC_INIT_TASK_STACK_SIZE  (1024 * 16)
#define USB_MSC_INIT_TASK_NAME        "msc_init"

#define TAG "udisk"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

bk_err_t usb_storage_enable(void)
{
    /* (c) Turn on USB PHY VCC 1.8V (ana_reg14 bit 10) + VCC 3V
     *     (ana_reg14 bit 11). This is the ONLY thing that brings
     *     up the analog rail on BK7259. */
    //LOGI("U-disk: enabling USB VCC 3V + VCC 1.8V via sys_ana_usb_phy_op(1)\n");
    //LOGI("        (ana_reg14 @0x%08lx |= bit10|bit11)\n",
    //     (unsigned long)(0x44010000UL + 0x4eUL * 4UL));
    //sys_ana_usb_phy_op(1);

    /* 4) Register the MSC class on top of the device controller.
     *    This also runs the SD-NAND bring-up via the V3P0 backend
     *    (gated by MSC_SD_BACKEND_AVAILABLE, see input.txt
     *    section 11.2). */
    LOGI("U-disk: calling msc_storage_init()\n");
    msc_storage_init();

    return BK_OK;
}

/* =======================================================================
 *  USB HOST mode: enumerate an inserted U-disk and do a file R/W test.
 *
 *  The board powers up as a USB *device* (MSC gadget) by default (see
 *  msc_storage_init() below). The `udisk` CLI lets a developer flip the
 *  same MUSB controller into *host* mode at runtime, enumerate a plugged
 *  U-disk, and run a FatFs write+read+verify round-trip on it.
 *
 *  Host bring-up uses the standard SDK path (the same one the UVC host
 *  camera uses): bk_usb_driver_init() once, then the OTG helper
 *  bk_usb_otg_manual_convers_mod() which internally tears down the device
 *  gadget and calls bk_usb_open(USB_HOST_MODE). FatFs reaches the U-disk
 *  through drive "2:" (DISK_NUMBER_UDISK -> driver_udisk.c ->
 *  usbh_device_read/write).
 * ===================================================================== */
#if (CONFIG_USB_HOST && CONFIG_USBH_MSC && CONFIG_FATFS)

/* No public header for the OTG helper; it is linked in whenever both
 * CONFIG_USB_DEVICE and CONFIG_USB_HOST are enabled. */
extern bk_err_t bk_usb_otg_manual_convers_mod(E_USB_MODE close_mod, E_USB_MODE open_mod);

#define UDISK_DRIVE_PATH        "2:"
#define UDISK_TEST_FILE         "2:/bk_udisk_test.txt"
#define UDISK_TEST_PAYLOAD      512u
#define UDISK_ENUM_WAIT_MS      8000u
#define UDISK_ENUM_POLL_MS      100u

static volatile int s_udisk_driver_inited;   /* bk_usb_driver_init() done once */
static volatile int s_udisk_in_host_mode;    /* 1 = host, 0 = device(default) */
static volatile int s_udisk_device_active = 1; /* device gadget is enabled at boot */

static bool udisk_is_media_ready(void)
{
    return s_udisk_in_host_mode && usbh_ms_media_get_status();
}

static void udisk_ensure_driver_init(void)
{
    if (!s_udisk_driver_inited) {
        bk_err_t ret = bk_usb_driver_init();
        LOGI("bk_usb_driver_init ret=%d\n", ret);
        s_udisk_driver_inited = 1;
    }
}

/* Wait for the host stack to finish SCSI INQUIRY + READ CAPACITY on the
 * inserted U-disk. usbh_ms_media_get_status() flips to non-zero once the
 * MSC class driver's connect callback has succeeded. */
static int udisk_wait_media(uint32_t timeout_ms)
{
    uint32_t waited = 0;
    while (waited < timeout_ms) {
        if (udisk_is_media_ready()) {
            LOGI("U-disk media READY after %u ms\n", (unsigned)waited);
            return 0;
        }
        rtos_delay_milliseconds(UDISK_ENUM_POLL_MS);
        waited += UDISK_ENUM_POLL_MS;
    }
    LOGW("U-disk media NOT ready after %u ms (no disk / enum failed)\n",
         (unsigned)timeout_ms);
    return -1;
}

static int udisk_switch_to_host(void)
{
    if (s_udisk_in_host_mode) {
        LOGI("already in HOST mode\n");
        return 0;
    }

    udisk_ensure_driver_init();

    bk_err_t ret;
    if (s_udisk_device_active) {
        LOGI("switching DEVICE -> HOST ...\n");
        /* close device gadget (msc_storage_deinit) + open host. */
        ret = bk_usb_otg_manual_convers_mod(USB_DEVICE_MODE, USB_HOST_MODE);
        if (ret != BK_OK) {
            LOGE("otg DEVICE->HOST failed ret=%d\n", ret);
            return -1;
        }
        s_udisk_device_active = 0;
    } else {
        /* Clean bring-up (no prior device mode), same as UVC host examples. */
        LOGI("opening HOST from clean state ...\n");
        ret = bk_usb_open(USB_HOST_MODE);
        if (ret != BK_OK) {
            LOGE("bk_usb_open(HOST) failed ret=%d\n", ret);
            return -1;
        }
    }
    s_udisk_in_host_mode = 1;
    LOGI("HOST mode active (VBUS supplied externally; plug U-disk into host port)\n");
    return 0;
}

static int udisk_switch_to_device(void)
{
    if (!s_udisk_in_host_mode) {
        LOGI("already in DEVICE mode\n");
        return 0;
    }

    LOGI("switching HOST -> DEVICE ...\n");
    bk_err_t ret = bk_usb_otg_manual_convers_mod(USB_HOST_MODE, USB_DEVICE_MODE);
    if (ret != BK_OK) {
        LOGE("otg HOST->DEVICE failed ret=%d\n", ret);
        return -1;
    }
    s_udisk_in_host_mode = 0;
    s_udisk_device_active = 1;
    LOGI("DEVICE (MSC gadget) mode restored\n");
    return 0;
}

#define UDISK_SCAN_MAX_DEPTH    4
#define UDISK_SCAN_PATH_MAX     256

/* Recursively walk one directory, printing every file and sub-directory with
 * indentation by depth and accumulating the total entry count via *count.
 * DIR/FILINFO are heap allocated per level (not on the PSRAM task stack) and the
 * recursion is bounded by UDISK_SCAN_MAX_DEPTH to keep memory/stack bounded. */
static FRESULT udisk_scan_dir(char *path, int depth, int *count)
{
    DIR *dir = (DIR *)os_malloc(sizeof(DIR));
    FILINFO *fno = (FILINFO *)os_malloc(sizeof(FILINFO));
    FRESULT fr = FR_NOT_ENOUGH_CORE;
    size_t base_len = os_strlen(path);

    if (!dir || !fno) {
        goto out;
    }
    fr = f_opendir(dir, path);
    if (fr != FR_OK) {
        LOGE("f_opendir(%s) failed fr=%d\n", path, fr);
        goto out;
    }
    while (1) {
        fr = f_readdir(dir, fno);
        if (fr != FR_OK || fno->fname[0] == 0) {
            break;
        }
        (*count)++;
        if (fno->fattrib & AM_DIR) {
            LOGI("%*s<DIR>  %s/\n", depth * 2, "", fno->fname);
            if (depth + 1 < UDISK_SCAN_MAX_DEPTH) {
                const char *sep = (base_len && path[base_len - 1] == '/') ? "" : "/";
                int n = os_snprintf(path + base_len,
                                    UDISK_SCAN_PATH_MAX - base_len,
                                    "%s%s", sep, fno->fname);
                if (n > 0 && (base_len + (size_t)n) < UDISK_SCAN_PATH_MAX) {
                    udisk_scan_dir(path, depth + 1, count);
                }
                path[base_len] = '\0';   /* restore parent path */
            }
        } else {
            LOGI("%*s<FIL>  %s  %lu B\n", depth * 2, "",
                 fno->fname, (unsigned long)fno->fsize);
        }
    }
    f_closedir(dir);

out:
    if (dir)  os_free(dir);
    if (fno)  os_free(fno);
    return fr;
}

static FRESULT udisk_scan_root(const char *title)
{
    char *path = (char *)os_malloc(UDISK_SCAN_PATH_MAX);
    FRESULT fr = FR_NOT_ENOUGH_CORE;
    int count = 0;

    if (!path) {
        return fr;
    }
    os_strcpy(path, UDISK_DRIVE_PATH "/");
    LOGI("---- U-disk file list (%s) ----\n", title ? title : "recursive");
    fr = udisk_scan_dir(path, 0, &count);
    LOGI("---- %d entries total ----\n", count);

    os_free(path);
    return fr;
}

/* Full headline test: switch to host, enumerate, mount, write a pattern,
 * read it back, verify, list the root directory, unmount. */
static int udisk_run_rw_test(void)
{
    FATFS  *fs = NULL;
    FIL    *fp = NULL;
    uint8_t *tx = NULL;
    uint8_t *rx = NULL;
    FRESULT fr;
    UINT bw = 0, br = 0;
    int  mounted = 0;
    int  rc = -1;

    LOGI("==== U-disk host R/W test BEGIN ====\n");

    if (udisk_switch_to_host() != 0) {
        return -1;
    }
    if (udisk_wait_media(UDISK_ENUM_WAIT_MS) != 0) {
        LOGE("no U-disk media; abort test\n");
        return -1;
    }

    fs = (FATFS *)os_malloc(sizeof(FATFS));
    fp = (FIL *)os_malloc(sizeof(FIL));
    tx = (uint8_t *)os_malloc(UDISK_TEST_PAYLOAD);
    rx = (uint8_t *)os_malloc(UDISK_TEST_PAYLOAD);
    if (!fs || !fp || !tx || !rx) {
        LOGE("os_malloc failed\n");
        goto out_free;
    }

    for (uint32_t i = 0; i < UDISK_TEST_PAYLOAD; i++) {
        tx[i] = (uint8_t)(i & 0xFF);
    }
    os_memcpy(tx, "BK7259-UDISK-HOST-RW-TEST\n", 26);

    fr = f_mount(fs, UDISK_DRIVE_PATH, 1);
    if (fr != FR_OK) {
        LOGE("f_mount(%s) failed fr=%d (3=NOT_READY, 13=NO_FILESYSTEM)\n",
             UDISK_DRIVE_PATH, fr);
        goto out_free;
    }
    mounted = 1;
    LOGI("mounted %s\n", UDISK_DRIVE_PATH);
    (void)udisk_scan_root("before write");

    /* write */
    fr = f_open(fp, UDISK_TEST_FILE, FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK) {
        LOGE("f_open(WR) failed fr=%d\n", fr);
        goto out_unmount;
    }
    fr = f_write(fp, tx, UDISK_TEST_PAYLOAD, &bw);
    (void)f_close(fp);
    if (fr != FR_OK || bw != UDISK_TEST_PAYLOAD) {
        LOGE("f_write failed fr=%d bw=%u\n", fr, (unsigned)bw);
        goto out_unmount;
    }
    LOGI("wrote %u bytes -> %s\n", (unsigned)bw, UDISK_TEST_FILE);

    /* read back + verify */
    fr = f_open(fp, UDISK_TEST_FILE, FA_READ);
    if (fr != FR_OK) {
        LOGE("f_open(RD) failed fr=%d\n", fr);
        goto out_unmount;
    }
    os_memset(rx, 0, UDISK_TEST_PAYLOAD);
    fr = f_read(fp, rx, UDISK_TEST_PAYLOAD, &br);
    (void)f_close(fp);
    if (fr != FR_OK || br != UDISK_TEST_PAYLOAD) {
        LOGE("f_read failed fr=%d br=%u\n", fr, (unsigned)br);
        goto out_unmount;
    }
    if (os_memcmp(tx, rx, UDISK_TEST_PAYLOAD) != 0) {
        LOGE("DATA MISMATCH!\n");
        goto out_unmount;
    }
    LOGI("read-back PASS (%u bytes verified)\n", (unsigned)br);

    (void)udisk_scan_root("after write");
    rc = 0;

out_unmount:
    if (mounted) {
        fr = f_mount(NULL, UDISK_DRIVE_PATH, 0);
        if (fr != FR_OK) {
            LOGW("f_unmount fr=%d\n", fr);
        } else {
            LOGI("unmounted %s\n", UDISK_DRIVE_PATH);
        }
    }
out_free:
    if (fs) os_free(fs);
    if (fp) os_free(fp);
    if (tx) os_free(tx);
    if (rx) os_free(rx);

    LOGI("==== U-disk host R/W test %s ====\n", rc == 0 ? "PASS" : "FAIL");
    return rc;
}

static const char *udisk_speed_str(uint8_t speed)
{
    switch (speed) {
    case USB_SPEED_LOW:  return "low (1.5Mbps)";
    case USB_SPEED_FULL: return "full (12Mbps)";
    case USB_SPEED_HIGH: return "high (480Mbps)";
    default:             return "unknown";
    }
}

/* Pretty-print the standard descriptors of one enumerated device: the device
 * descriptor, then every interface and its endpoints. */
static void udisk_print_hubport(struct usbh_hubport *hport)
{
    const struct usb_device_descriptor *dd = &hport->device_desc;

    LOGI("  [dev addr %u] speed=%s\n", hport->dev_addr, udisk_speed_str(hport->speed));
    LOGI("    VID:PID=%04x:%04x bcdUSB=%04x bcdDevice=%04x\n",
         dd->idVendor, dd->idProduct, dd->bcdUSB, dd->bcdDevice);
    LOGI("    class=%02x sub=%02x proto=%02x ep0_mps=%u numConfig=%u\n",
         dd->bDeviceClass, dd->bDeviceSubClass, dd->bDeviceProtocol,
         dd->bMaxPacketSize0, dd->bNumConfigurations);
    if (hport->iManufacturer) LOGI("    Manufacturer: %s\n", hport->iManufacturer);
    if (hport->iProduct)      LOGI("    Product     : %s\n", hport->iProduct);
    if (hport->iSerialNumber) LOGI("    Serial      : %s\n", hport->iSerialNumber);

    uint8_t nif = hport->config.config_desc.bNumInterfaces;
    if (nif > CONFIG_USBHOST_MAX_INTERFACES) {
        nif = CONFIG_USBHOST_MAX_INTERFACES;
    }
    LOGI("    interfaces=%u\n", nif);
    for (uint8_t i = 0; i < nif; i++) {
        const struct usb_interface_descriptor *id =
            &hport->config.intf[i].altsetting[0].intf_desc;
        LOGI("    if[%u] class=%02x sub=%02x proto=%02x numEp=%u\n",
             i, id->bInterfaceClass, id->bInterfaceSubClass,
             id->bInterfaceProtocol, id->bNumEndpoints);
        uint8_t nep = id->bNumEndpoints;
        if (nep > CONFIG_USBHOST_MAX_ENDPOINTS) {
            nep = CONFIG_USBHOST_MAX_ENDPOINTS;
        }
        for (uint8_t e = 0; e < nep; e++) {
            const struct usb_endpoint_descriptor *ed =
                &hport->config.intf[i].altsetting[0].ep[e].ep_desc;
            LOGI("      ep 0x%02x attr=0x%02x mps=%u interval=%u\n",
                 ed->bEndpointAddress, ed->bmAttributes,
                 ed->wMaxPacketSize, ed->bInterval);
        }
    }
}

/* Scan enumerated devices and print each one. Returns the number of devices found. */
static int udisk_print_enumerated_devices(void)
{
    int found = 0;
#if CONFIG_BK_USB_CHERRYUSB_V1_6
    /* CherryUSB v1.6 changed usbh_find_hubport() from dev_addr based lookup to
     * (busid, hub_index, hub_port). The public SDK compatibility header still
     * declares the legacy one-argument form, so call the v1.6 ABI through a
     * local function pointer and scan the known hub/port matrix.
     *
     * hub_index 1 is the root hub. External hub indices start at 2; scan one
     * more level so the "USB HUB + U-disk" case is covered. */
    struct usbh_hubport *(*find_hubport)(uint8_t, uint8_t, uint8_t) =
        (struct usbh_hubport *(*)(uint8_t, uint8_t, uint8_t))usbh_find_hubport;
    const uint8_t max_hub_index = 1 + CONFIG_USBHOST_MAX_EXTHUBS;

    for (uint8_t hub_index = 1; hub_index <= max_hub_index; hub_index++) {
        uint8_t max_port = (hub_index == 1) ? CONFIG_USBHOST_MAX_RHPORTS : CONFIG_USBHOST_MAX_EHPORTS;

        for (uint8_t port = 1; port <= max_port; port++) {
            struct usbh_hubport *hport = find_hubport(0, hub_index, port);
            if (hport && hport->connected) {
                udisk_print_hubport(hport);
                found++;
            }
        }
    }
#else
    for (uint8_t addr = 1; addr <= 8; addr++) {
        struct usbh_hubport *hport = usbh_find_hubport(addr);
        if (hport && hport->connected) {
            udisk_print_hubport(hport);
            found++;
        }
    }
#endif
    return found;
}

/* Enumeration case: switch to host, wait for an inserted device to enumerate,
 * then print its descriptors. Does NOT mount a filesystem (works for any USB
 * device class, not just U-disk). */
static int udisk_run_enum(void)
{
    LOGI("==== USB host enumeration BEGIN ====\n");
    if (udisk_switch_to_host() != 0) {
        LOGE("cannot enter host mode\n");
        LOGI("==== USB host enumeration FAIL ====\n");
        return -1;
    }

    LOGI("waiting for a device to be plugged in / enumerated ...\n");
    int devs = 0;
    uint32_t waited = 0;
    while (waited < UDISK_ENUM_WAIT_MS) {
        devs = udisk_print_enumerated_devices();
        if (devs > 0) {
            break;
        }
        rtos_delay_milliseconds(UDISK_ENUM_POLL_MS);
        waited += UDISK_ENUM_POLL_MS;
    }

    if (devs == 0) {
        LOGW("no device enumerated within %u ms (check cable / power / speed)\n",
             (unsigned)UDISK_ENUM_WAIT_MS);
        LOGI("==== USB host enumeration FAIL ====\n");
        return -1;
    }

    LOGI("enumerated %d device(s)\n", devs);
    LOGI("==== USB host enumeration PASS ====\n");
    return 0;
}

#if CONFIG_CLI
static void cli_udisk_help(void)
{
    LOGI("usage:\n");
    LOGI("  udisk host    - switch USB to HOST and enumerate the U-disk\n");
    LOGI("  udisk dev     - switch USB back to DEVICE (MSC gadget, default)\n");
    LOGI("  udisk enum    - host + wait for inserted device + print descriptors\n");
    LOGI("  udisk test    - host + enumerate + mount + write/read/verify + ls\n");
    LOGI("  udisk ls      - list root dir of the mounted U-disk\n");
    LOGI("  udisk status  - print current mode + media status\n");
}

static void cli_udisk_status(void)
{
    LOGI("mode=%s, driver_init=%d, host_media=%s\n",
         s_udisk_in_host_mode ? "host" : "device",
         s_udisk_driver_inited,
         udisk_is_media_ready() ? "ready" : "not_ready");
}

static void cli_udisk_cmd(char *pcWriteBuffer, int xWriteBufferLen,
                          int argc, char **argv)
{
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;

    if (argc < 2) {
        cli_udisk_help();
        cli_udisk_status();
        return;
    }

    const char *sub = argv[1];
    if (os_strcmp(sub, "host") == 0) {
        if (udisk_switch_to_host() == 0) {
            (void)udisk_wait_media(UDISK_ENUM_WAIT_MS);
        }
        cli_udisk_status();
    } else if (os_strcmp(sub, "dev") == 0) {
        (void)udisk_switch_to_device();
        cli_udisk_status();
    } else if (os_strcmp(sub, "enum") == 0) {
        (void)udisk_run_enum();
    } else if (os_strcmp(sub, "test") == 0) {
        (void)udisk_run_rw_test();
    } else if (os_strcmp(sub, "ls") == 0) {
        (void)udisk_scan_root("manual ls");
    } else if (os_strcmp(sub, "status") == 0) {
        cli_udisk_status();
    } else {
        cli_udisk_help();
    }
}

/* Register through the linker .cli_cmdtabl section so bk_cli_init() picks the
 * `udisk` command up automatically, independent of which core runs main() and
 * of CLI-subsystem init ordering. The previous explicit cli_register_commands()
 * from main() raced the CLI bring-up and (because msc_init/main float between
 * ap0/ap1) intermittently left `udisk` missing from the console table
 * ("cmd NOT found: udisk"). */
COMPONENTS_CLI_CMD_EXPORT
static const struct cli_command s_udisk_cmds[] = {
    {"udisk", "udisk host|dev|enum|test|ls|status", cli_udisk_cmd},
};

static void udisk_cli_register(void)
{
    /* No-op: s_udisk_cmds is auto-registered via the .cli_cmdtabl section. */
    LOGI("udisk CLI available via auto-register (try: udisk status)\n");
}
#else  /* !CONFIG_CLI */
static void udisk_cli_register(void) {}
#endif /* CONFIG_CLI */

#else  /* host MSC not enabled */
static void udisk_cli_register(void) {}
#endif /* CONFIG_USB_HOST && CONFIG_USBH_MSC && CONFIG_FATFS */

/* =======================================================================
 *  USB DEVICE mode: MTP (Media Transfer Protocol) gadget.
 *
 *  The board powers up as a USB MSC gadget (msc_storage_init). MTP is an
 *  alternative *device* gadget that exposes the on-board SD-card filesystem
 *  to a PC as a media device (browse / copy files in Explorer / mtp-tools).
 *
 *  The MTP engine (ap/components/bk_usb/bk_mtp/bk_usbd_mtp.c) is ported from
 *  the glass project and adapted to the CherryUSB v1.6 device API. It talks
 *  to storage through the bk_vfs POSIX layer (opendir/readdir/open/...), so
 *  we mount the SD card at VFS_SD_0_PATITION_0 ("/sd0") before bring-up.
 *
 *  `mtp start` swaps the MSC gadget for the MTP gadget on the same USB
 *  controller (busid 0). usbd_desc_register() memset()s the device core, so
 *  re-registering MTP after deinit-ing MSC is clean.
 * ===================================================================== */
#if CONFIG_USBD_MTP
#include "bk_usb_mtp.h"

static volatile int s_mtp_active;

static int mtp_start(void)
{
    if (s_mtp_active) {
        LOGI("MTP already active\n");
        return 0;
    }

#if (CONFIG_USB_HOST && CONFIG_USBH_MSC && CONFIG_FATFS)
    /* MTP is a device gadget; if we are currently in host mode, go back. */
    if (s_udisk_in_host_mode) {
        (void)udisk_switch_to_device();
    }
#endif

    /* Tear down the default MSC gadget so MTP can own busid 0. The MTP engine
     * (usb_mtp_init) mounts the SD card at /sd0 itself before bring-up. */
    LOGI("MTP: deinit MSC gadget\n");
    (void)msc_storage_deinit();

    int ret = usb_mtp_init();
    LOGI("MTP: usb_mtp_init ret=%d\n", ret);
    s_mtp_active = (ret == 0) ? 1 : 0;
    if (s_mtp_active) {
        LOGI("==== MTP device active (browse the SD card on the PC) ====\n");
    }
    return ret;
}

static int mtp_stop(void)
{
    if (!s_mtp_active) {
        LOGI("MTP not active\n");
        return 0;
    }
    int ret = usb_mtp_deinit();
    LOGI("MTP: usb_mtp_deinit ret=%d\n", ret);
    s_mtp_active = 0;
    return ret;
}

#if CONFIG_CLI
static void cli_mtp_help(void)
{
    LOGI("usage:\n");
    LOGI("  mtp start   - swap MSC->MTP gadget, mount SD, enumerate as MTP\n");
    LOGI("  mtp stop    - tear down the MTP gadget\n");
    LOGI("  mtp status  - print current MTP state\n");
}

static void cli_mtp_cmd(char *pcWriteBuffer, int xWriteBufferLen,
                        int argc, char **argv)
{
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;

    if (argc < 2) {
        cli_mtp_help();
        LOGI("mtp active=%d\n", s_mtp_active);
        return;
    }

    const char *sub = argv[1];
    if (os_strcmp(sub, "start") == 0) {
        (void)mtp_start();
    } else if (os_strcmp(sub, "stop") == 0) {
        (void)mtp_stop();
    } else if (os_strcmp(sub, "status") == 0) {
        LOGI("mtp active=%d\n", s_mtp_active);
    } else {
        cli_mtp_help();
    }
}

COMPONENTS_CLI_CMD_EXPORT
static const struct cli_command s_mtp_cmds[] = {
    {"mtp", "mtp start|stop|status", cli_mtp_cmd},
};
#endif /* CONFIG_CLI */
#endif /* CONFIG_USBD_MTP */

static void msc_storage_init_task(void *arg)
{
    (void)arg;

    bk_err_t ret = 0;

    GPIO_DOWN(GPIO_32);
    GPIO_DOWN(GPIO_33);
    GPIO_DOWN(GPIO_34);
    GPIO_DOWN(GPIO_35);
    GPIO_DOWN(GPIO_36);
    GPIO_DOWN(GPIO_37);
    GPIO_DOWN(GPIO_38);
    GPIO_DOWN(GPIO_39);
    GPIO_DOWN(GPIO_40);
    GPIO_DOWN(GPIO_41);
    GPIO_DOWN(GPIO_42);
    GPIO_DOWN(GPIO_43);
    GPIO_DOWN(GPIO_44);
    GPIO_DOWN(GPIO_45);
    GPIO_DOWN(GPIO_46);
    GPIO_DOWN(GPIO_47);
    GPIO_DOWN(GPIO_48);

    LOGI("[%s] msc_init: ENTER\r\n", TAG);
    ret = usb_storage_enable();
    LOGI("[%s] msc_init: RET=%d\r\n", TAG, ret);

    /* CLI now registered from main(); nothing to do here. */

    /* Bring-up done. The CherryUSB "usbd_msc" worker thread keeps
     * the MSC pipeline alive on its own; the usb_dbg watchdog
     * (spawned inside usb_storage_enable()) handles "is it still
     * running?" diagnostics from here on out. This task has no
     * steady-state work, so self-delete to release its stack/TCB
     * back to FreeRTOS. */
    LOGI("[%s] msc_init: task done, self-deleting\r\n", TAG);
    s_msc_init_thread = NULL;
    rtos_delete_thread(NULL);
}

int main(void)
{
    bk_init();

    BK_LOGI(NULL, "AP main running...\r\n");

    /* Register the `udisk` host-test CLI from main() (console core, after the
     * CLI subsystem is up). Doing it here instead of from the early msc_init
     * task avoids a registration race that left `udisk` missing from the
     * shell command table. */
    udisk_cli_register();

#if CONFIG_VOICE_SERVICE_TEST
    /* Register the `voice` CLI so the UAC mic/speaker host path can be exercised
     * on usb_example (the UAC ISO OUT submit -EBUSY case lives here), e.g.:
     *   ap_cmd voice start uac 16000 1 pcm pcm uac 16000
     * cli_voice_init() is part of bk_voice_service (compiled when
     * CONFIG_VOICE_SERVICE_TEST=y); usb_example simply never registered it. */
    {
        extern int cli_voice_init(void);
        cli_voice_init();
    }
#endif

    bk_err_t cret = rtos_create_thread(&s_msc_init_thread,
        USB_MSC_INIT_TASK_PRIORITY,
        USB_MSC_INIT_TASK_NAME,
        (beken_thread_function_t)msc_storage_init_task,
        USB_MSC_INIT_TASK_STACK_SIZE,
        NULL);
    if (cret != BK_OK) {
        LOGE("[%s] create %s task FAILED ret=%d, skipping MSC bring-up\r\n", TAG, USB_MSC_INIT_TASK_NAME, cret);
        return -1;
   }

    return 0;
}
