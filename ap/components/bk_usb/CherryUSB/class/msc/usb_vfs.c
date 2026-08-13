/*
 * USB MSC <-> local FatFs/LittleFs arbitration.
 *
 * Restored from the original bk_ai_release_2.0.1 SDK (which shipped
 * the same helpers under the name `usb_vfs_*`) to fill the gap left
 * in the BK7259 SDK -- usbd_msc.c here still calls into lv_vfs_init
 * / lv_vfs_deinit but the implementation file was dropped from the
 * tree. See usb_vfs.h for the full call-site rationale.
 *
 * Compiled only when CONFIG_USBD_MSC && CONFIG_VFS are both on (see
 * the CMakeLists.txt guard); without CONFIG_VFS the call sites in
 * usbd_msc.c are #if'd out and nobody references these symbols. The
 * inner #if CONFIG_VFS guard below is defensive: it keeps the TU
 * trivially empty if somebody adds this file to a build without
 * CONFIG_VFS by mistake.
 */
#include "usb_vfs.h"

#if CONFIG_VFS

#include <os/os.h>
#include <os/mem.h>
#include <components/system.h>
#include <common/bk_include.h>
#include "bk_posix.h"
#include "bk_partition.h"

#if CONFIG_LITTLEFS
#include "driver/flash_partition.h"
#endif

#if CONFIG_USBD_MSC_STORAGE_QSPI_NAND
#include <driver/nand_ftl.h>
#define USB_VFS_NAND_QSPI_ID    QSPI_ID_0
#endif

#define USB_VFS_MOUNT_POINT     "/"
#define USB_VFS_TAG             "usb_vfs"

#if CONFIG_FATFS && CONFIG_USBD_MSC_STORAGE_QSPI_NAND
/* FatFs-on-QSPI-NAND (via the Dhara FTL) is the local mount that shares the
 * same block device as the USB MSC LUN. Only one side may touch the FTL at a
 * time; the U-disk handover drops this mount (after syncing) before MSC goes
 * live, and restores it (re-reading a fresh FAT) once the host releases. */
static int _fs_mount_fatfs_nand(void)
{
    struct bk_fatfs_partition partition = { 0 };

    partition.part_type             = FATFS_DEVICE;
    /* QSPI-NAND is served on the QSPI-0 drive via the Dhara FTL; the device
     * name is the reused qspi0_flash slot (disk_io.c routes it to the FTL when
     * CONFIG_QSPI_NAND_FLASH is set). */
    partition.part_dev.device_name  = FATFS_DEV_QSPI0_FLASH;
    partition.mount_path            = USB_VFS_MOUNT_POINT;

    return mount("SOURCE_NONE", partition.mount_path, "fatfs", 0, &partition);
}
#endif /* CONFIG_FATFS && CONFIG_USBD_MSC_STORAGE_QSPI_NAND */

#if CONFIG_FATFS
/* FatFs-on-SD-card is the primary backing on the robot V1 AI kit
 * (CONFIG_SDCARD=y / CONFIG_SDIO_V3P0=y at the board level + the
 * MKDV4GCL-ABB SD-NAND on SDIO controller 1). When the U-disk
 * button hands the card over to USB MSC we drop this mount, and
 * when the host releases (MSC suspend) we put it back. */
static int _fs_mount_fatfs(void)
{
    struct bk_fatfs_partition partition = { 0 };

    partition.part_type             = FATFS_DEVICE;
    partition.part_dev.device_name  = FATFS_DEV_SDCARD;
    partition.mount_path            = USB_VFS_MOUNT_POINT;

    /* "SOURCE_NONE" is the documented sentinel for FatFs (see
     * cli_vfs.c). The 4th arg (mount_flags) is unused by the FatFs
     * adapter today; pass 0 for forward compatibility. */
    return mount("SOURCE_NONE", partition.mount_path, "fatfs", 0, &partition);
}
#endif /* CONFIG_FATFS */

#if (!CONFIG_FATFS) && CONFIG_LITTLEFS
/* Fallback path: no FatFs in this build, so put the mount on
 * LittleFs over a flash partition instead. Kept structurally
 * identical to the legacy SDK so behaviour matches whatever the
 * original USB MSC bring-up did on flash-only targets. */
static int _fs_mount_littlefs(void)
{
    struct bk_little_fs_partition partition = { 0 };
    bk_logic_partition_t          *pt;

#ifdef BK_PARTITION_LITTLEFS_USER
    pt = bk_flash_partition_get_info(BK_PARTITION_LITTLEFS_USER);
#else
    pt = bk_flash_partition_get_info(BK_PARTITION_USR_CONFIG);
#endif
    if (pt == NULL) {
        return -1;
    }

    partition.part_type             = LFS_FLASH;
    partition.part_flash.start_addr = pt->partition_start_addr;
    partition.part_flash.size       = pt->partition_length;
    partition.mount_path            = USB_VFS_MOUNT_POINT;

    return mount("SOURCE_NONE", partition.mount_path, "littlefs", 0, &partition);
}
#endif /* !CONFIG_FATFS && CONFIG_LITTLEFS */

static int _fs_mount(void)
{
#if CONFIG_FATFS && CONFIG_USBD_MSC_STORAGE_QSPI_NAND
    return _fs_mount_fatfs_nand();
#elif CONFIG_FATFS
    return _fs_mount_fatfs();
#elif CONFIG_LITTLEFS
    return _fs_mount_littlefs();
#else
    /* Neither FatFs nor LittleFs compiled in: nothing to arbitrate
     * with. msc_storage_init() will still bring the USB device up;
     * we just don't have a local mount to drop. */
    return 0;
#endif
}

bk_err_t lv_vfs_init(void)
{
    int ret = _fs_mount();
    if (ret != 0) {
        bk_printf("[%s] mount fail:%d\r\n", USB_VFS_TAG, ret);
        return BK_FAIL;
    }
    bk_printf("[%s] mount success\r\n", USB_VFS_TAG);
    return BK_OK;
}

bk_err_t lv_vfs_deinit(void)
{
#if CONFIG_FATFS || CONFIG_LITTLEFS
    int ret = umount(USB_VFS_MOUNT_POINT);
    if (ret != 0) {
        bk_printf("[%s] umount fail:%d\r\n", USB_VFS_TAG, ret);
        return BK_FAIL;
    }
    bk_printf("[%s] umount success\r\n", USB_VFS_TAG);
#endif
#if CONFIG_USBD_MSC_STORAGE_QSPI_NAND
    /* Local FatFs is now detached; make the FTL durable before the host owns
     * the block device so MSC starts from a consistent, flushed image. */
    (void)bk_nand_ftl_sync(USB_VFS_NAND_QSPI_ID);
#endif
    return BK_OK;
}

#else /* !CONFIG_VFS */

bk_err_t lv_vfs_init(void)   { return BK_OK; }
bk_err_t lv_vfs_deinit(void) { return BK_OK; }

#endif /* CONFIG_VFS */
