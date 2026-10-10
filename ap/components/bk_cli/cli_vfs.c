#include "cli.h"

#if CONFIG_VFS

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <cli.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "bk_posix.h"

#include "driver/flash_partition.h"
#include <driver/aon_rtc.h>
#include "CheckSumUtils.h"
#if CONFIG_TASK_WDT
#include "bk_wdt.h"
#endif

#if CONFIG_QSPI_NOR_FLASH || CONFIG_QSPI_NAND_FLASH
/* Needed for both media: also supplies SOC_QSPI_UNIT_NUM (via the SoC qspi
 * caps), which decides whether this part has a second QSPI unit. */
#include <driver/qspi_types.h>
#endif

#if CONFIG_QSPI_NAND_FLASH
/* qspi_flash.h uses qspi_id_t without pulling in its definition. */
#include <driver/qspi_flash.h>
#include <driver/qspi_nand_bbm.h>
#endif

#if CONFIG_DHARA_FTL
#include <driver/nand_ftl.h>
#endif

#if defined(BK_PARTITION_LITTLEFS)
#define BK_PARTITION_FS_ID BK_PARTITION_LITTLEFS
#else
#define BK_PARTITION_FS_ID BK_PARTITION_USR_CONFIG
#endif

static int test_format_lfs(void) {
	struct bk_little_fs_partition partition;
	char *fs_name = NULL;

	int ret;

	bk_logic_partition_t *fs_partition = bk_flash_partition_get_info(BK_PARTITION_FS_ID);
	uint32_t flash_start = fs_partition->partition_start_addr;
	uint32_t flash_size = fs_partition->partition_length;
	fs_name = "littlefs";
	partition.part_type = LFS_FLASH;
	partition.part_flash.start_addr = flash_start;
	partition.part_flash.size = flash_size;

	ret = mkfs("PART_NONE", fs_name, &partition);
	
	return ret;
}

static int test_mount_lfs(char *mount_point) {
	struct bk_little_fs_partition partition;
	char *fs_name = NULL;

	int ret;
	bk_logic_partition_t *fs_partition = bk_flash_partition_get_info(BK_PARTITION_FS_ID);
	uint32_t flash_start = fs_partition->partition_start_addr;
	uint32_t flash_size = fs_partition->partition_length;
	fs_name = "littlefs";
	partition.part_type = LFS_FLASH;
	partition.part_flash.start_addr = flash_start;
	partition.part_flash.size = flash_size;
	partition.mount_path = mount_point;

	ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

	return ret;
}

#define spi_flash_start 0x1000
#define spi_flash_size	0x100000		// 1 MB

static int test_format_spi_lfs(void) {
	struct bk_little_fs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "littlefs";
	partition.part_type = LFS_SPI_FLASH;
	partition.part_flash.start_addr = spi_flash_start;
	partition.part_flash.size = spi_flash_size;

	ret = mkfs("PART_NONE", fs_name, &partition);
	
	return ret;
}

static int test_mount_spi_lfs(char *mount_point) {
	struct bk_little_fs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "littlefs";
	partition.part_type = LFS_SPI_FLASH;
	partition.part_flash.start_addr = spi_flash_start;
	partition.part_flash.size = spi_flash_size;
	partition.mount_path = mount_point;

	ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

	return ret;
}

/*
 * QSPI littlefs test partitions.
 * NAND erase unit is a 128KB block, so partitions must be block-aligned and big
 * enough for littlefs (use 1MB = 8 blocks each). NOR keeps the original 256KB.
 *
 * p0 deliberately starts past the first 8 NAND blocks. Those are the only ones
 * the datasheet guarantees valid at shipment (parameter page byte 107 = 08h),
 * and a soak run concentrates every erase cycle on this partition - roughly
 * 2200 P/E per block per hour measured at 620 KB/s into 1MB - so leaving them
 * untouched keeps a pristine reference region on every sample.
 */
#if CONFIG_QSPI_NAND_FLASH
#define qspi_lfs_p0_start	0x800000	// 8 MB (NAND block 64)
#define qspi_lfs_p0_size	0x100000	// 1 MB (8 NAND blocks)
#define qspi_lfs_p1_start	0x900000	// 9 MB (NAND block 72), right after p0
#define qspi_lfs_p1_size	0x100000
#else
#define qspi_lfs_p0_start	0
#define qspi_lfs_p0_size	0x40000
#define qspi_lfs_p1_start	0x80000
#define qspi_lfs_p1_size	0x40000
#endif

static int test_format_qspi_lfs(void) {
	struct bk_little_fs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "littlefs";
	partition.part_type = LFS_QSPI_FLASH;
	partition.part_flash.start_addr = qspi_lfs_p0_start;
	partition.part_flash.size = qspi_lfs_p0_size;

	ret = mkfs("PART_NONE", fs_name, &partition);

	return ret;
}

static int test_mount_qspi_lfs(char *mount_point) {
	struct bk_little_fs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "littlefs";
	partition.part_type = LFS_QSPI_FLASH;
	partition.part_flash.start_addr = qspi_lfs_p0_start;
	partition.part_flash.size = qspi_lfs_p0_size;
	partition.mount_path = mount_point;

	ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

	return ret;
}

static int test_format_qspi0_part1_lfs(void) {
	struct bk_little_fs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "littlefs";
	partition.part_type = LFS_QSPI_FLASH;
	partition.part_flash.start_addr = qspi_lfs_p1_start;
	partition.part_flash.size = qspi_lfs_p1_size;

	ret = mkfs("PART_NONE", fs_name, &partition);

	return ret;
}

static int test_mount_qspi0_part1_lfs(char *mount_point) {
	struct bk_little_fs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "littlefs";
	partition.part_type = LFS_QSPI_FLASH;
	partition.part_flash.start_addr = qspi_lfs_p1_start;
	partition.part_flash.size = qspi_lfs_p1_size;
	partition.mount_path = mount_point;

	ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

	return ret;
}

static int test_format_fatfs(void) {
	struct bk_fatfs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "fatfs";
	partition.part_type = FATFS_DEVICE;
	partition.part_dev.device_name = FATFS_DEV_SDCARD;

	ret = mkfs("PART_NONE", fs_name, &partition);
	
	return ret;
}

static int test_mount_fatfs(char *mount_point) {
	struct bk_fatfs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "fatfs";
	partition.part_type = FATFS_DEVICE;
	partition.part_dev.device_name = FATFS_DEV_SDCARD;
	partition.mount_path = mount_point;

	ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

	return ret;
}

static int test_format_flash_fatfs(void) {
	struct bk_fatfs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "fatfs";
	partition.part_type = FATFS_DEVICE;
	partition.part_dev.device_name = FATFS_DEV_FLASH;

	ret = mkfs("PART_NONE", fs_name, &partition);
	
	return ret;
}

static int test_mount_flash_fatfs(char *mount_point) {
	struct bk_fatfs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "fatfs";
	partition.part_type = FATFS_DEVICE;
	partition.part_dev.device_name = FATFS_DEV_FLASH;
	partition.mount_path = mount_point;

	ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

	return ret;
}

static int test_format_qspi0_fatfs(void) {
	struct bk_fatfs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "fatfs";
	partition.part_type = FATFS_DEVICE;
	partition.part_dev.device_name = FATFS_DEV_QSPI0_FLASH;

	ret = mkfs("PART_NONE", fs_name, &partition);
	
	return ret;
}

static int test_mount_qspi0_fatfs(char *mount_point) {
	struct bk_fatfs_partition partition;
	char *fs_name = NULL;

	int ret;

	fs_name = "fatfs";
	partition.part_type = FATFS_DEVICE;
	partition.part_dev.device_name = FATFS_DEV_QSPI0_FLASH;
	partition.mount_path = mount_point;

	ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

	return ret;
}

static int test_umount_vfs(char *mount_point) {
	int ret;
	
	ret = umount(mount_point);
	return ret;
}

static int show_file(int fd) {
	#define CHUNK 18
	char buffer[CHUNK+1];
	int len;
	int total = 0;
	char *ptr;
	char *ptr2;

	buffer[CHUNK] = '\0';
	while(1) {
		len = read(fd, buffer, CHUNK);
		if (len <= 0)
			break;

		if (len == 0)
			break;

		total += len;

		ptr = buffer;
		while(ptr < buffer + len) {
			ptr2 = strchr(ptr, '\0');
			if (!ptr2)	//impossible
				break;
			if (ptr2 < buffer + len) {
				BK_LOGD(NULL, "%s\n", ptr);
			} else {
				BK_LOGD(NULL, "%s", ptr);
			}
			ptr = ptr2 + 1;
		}
	}

	return total;
	#undef CHUNK
}

static int test_read_vfs(char *file_name)
{
	int fd;
	int ret;

	fd = open(file_name, O_RDONLY);
	if (fd < 0) {
		BK_LOGD(NULL, "can't open %s\n", file_name);
		return -1;
	}
	
	ret = show_file(fd);
	//BK_LOGD(NULL, "read from %s, ret=%d\n", file_name, ret);
	close(fd);
	return ret;
}

static int test_write_vfs(char *file_name, char *content)
{
	int fd;
	int ret;

	fd = open(file_name, O_RDWR | O_CREAT | O_APPEND);
	if (fd < 0) {
		BK_LOGD(NULL, "can't open %s\n", file_name);
		return -1;
	}
	
	ret = write(fd, content, strlen(content) + 1);
	//BK_LOGD(NULL, "write to %s, ret=%d\n", file_name, ret);
	close(fd);
	return ret;
}

static int test_unlink_vfs(char *file_name)
{
	int ret;

	ret = unlink(file_name);
	return ret;
}

static int test_rename_vfs(char *old_name, char *new_name)
{
	return rename(old_name, new_name);
}

/* ==========================================================================
 * Throughput + data-integrity tests: `vfs bench`, `vfs soak`,
 * `vfs threadsafe_test`. All three share one test core (vfs_bench_round) and
 * reuse the mkfs/mount helpers above, so the partition geometry is defined
 * exactly once.
 * ========================================================================== */

/* IO unit. Big enough to amortise the per-call VFS/FS overhead so the numbers
 * reflect the medium, small enough to stay off the 4KB-ish heap cliff. */
#define VFS_BENCH_CHUNK          4096U

#define VFS_BENCH_DEF_FILE_KB    256U
#define VFS_BENCH_DEF_ROUNDS     4U
#define VFS_BENCH_MAX_FILE_KB    (16U * 1024U)

#define VFS_SOAK_REPORT_ROUNDS   16U     /* rounds between periodic reports */
#define VFS_SOAK_IDLE_MS         20U     /* yield between rounds */

#define VFS_TS_MAX_TASKS         8U
#define VFS_TS_DEF_FILE_KB       64U

#define VFS_BENCH_DEF_DEV        "qspi0"

/* Which filesystem to put on the device for a run. */
typedef enum {
	VFS_BENCH_FS_LFS = 0,
	VFS_BENCH_FS_FATFS,
	VFS_BENCH_FS_MAX,
} vfs_bench_fs_t;

/*
 * Storage device table.
 *
 * The medium (which interface/chip) and the filesystem (littleFS vs FatFs) are
 * orthogonal in the VFS: fs_type picks the filesystem, the partition struct
 * picks the medium. The table keeps that split so a new medium is one row here
 * rather than a new pair of CLI subcommands.
 *
 * Every device is always listed, even when the build cannot use it. A row
 * whose lfs_part_type is VFS_DEV_NO_LFS or whose fatfs_dev is NULL was compiled
 * out, and the command says so instead of reporting an unknown device name.
 *
 * NOR vs NAND is deliberately not a column: CONFIG_QSPI_NOR_FLASH and
 * CONFIG_QSPI_NAND_FLASH are mutually exclusive build-time options, so it
 * cannot be chosen at runtime.
 */
#define VFS_DEV_NO_LFS  (-1)

#if CONFIG_SPI_MST_FLASH
#define VFS_DEV_LFS_SPI0    LFS_SPI_FLASH
#define VFS_DEV_LFS_SPI1    LFS_SPI_1_FLASH
#else
#define VFS_DEV_LFS_SPI0    VFS_DEV_NO_LFS
#define VFS_DEV_LFS_SPI1    VFS_DEV_NO_LFS
#endif

#if CONFIG_QSPI_NOR_FLASH || CONFIG_QSPI_NAND_FLASH
#ifndef SOC_QSPI_UNIT_NUM
/* Without this the (SOC_QSPI_UNIT_NUM > 1) test below would quietly evaluate to
 * 0 and drop qspi1 from the table, which is far worse than failing here. */
#error "SOC_QSPI_UNIT_NUM undefined: driver/qspi_types.h did not supply the QSPI caps"
#endif
#define VFS_DEV_LFS_QSPI0   LFS_QSPI_FLASH
#if (SOC_QSPI_UNIT_NUM > 1)
#define VFS_DEV_LFS_QSPI1   LFS_QSPI_1_FLASH
#define VFS_DEV_QSPI1_ID    1
#else
#define VFS_DEV_LFS_QSPI1   VFS_DEV_NO_LFS
#define VFS_DEV_QSPI1_ID    (-1)
#endif
#else
#define VFS_DEV_LFS_QSPI0   VFS_DEV_NO_LFS
#define VFS_DEV_LFS_QSPI1   VFS_DEV_NO_LFS
#define VFS_DEV_QSPI1_ID    (-1)
#endif

#if CONFIG_FATFS_INTERNAL_FLASH
#define VFS_DEV_FAT_FLASH   FATFS_DEV_FLASH
#else
#define VFS_DEV_FAT_FLASH   NULL
#endif
#if CONFIG_FATFS_SPI_0_FLASH
#define VFS_DEV_FAT_SPI0    FATFS_DEV_SPI0_FLASH
#else
#define VFS_DEV_FAT_SPI0    NULL
#endif
#if CONFIG_FATFS_SPI_1_FLASH
#define VFS_DEV_FAT_SPI1    FATFS_DEV_SPI1_FLASH
#else
#define VFS_DEV_FAT_SPI1    NULL
#endif
#if CONFIG_FATFS_QSPI_0_FLASH
#define VFS_DEV_FAT_QSPI0   FATFS_DEV_QSPI0_FLASH
#else
#define VFS_DEV_FAT_QSPI0   NULL
#endif
#if CONFIG_FATFS_QSPI_1_FLASH
#define VFS_DEV_FAT_QSPI1   FATFS_DEV_QSPI1_FLASH
#else
#define VFS_DEV_FAT_QSPI1   NULL
#endif
#if CONFIG_FATFS_SDCARD
#define VFS_DEV_FAT_SD      FATFS_DEV_SDCARD
#else
#define VFS_DEV_FAT_SD      NULL
#endif

typedef struct {
	const char *name;          /* device name used on the command line */
	const char *mount_point;   /* default mount point for this medium */
	int         lfs_part_type; /* VFS_DEV_NO_LFS when littleFS is unavailable */
	uint32_t    lfs_start;     /* littleFS partition; size 0 => ask the */
	uint32_t    lfs_size;      /* partition table at runtime */
	const char *fatfs_dev;     /* NULL when FatFs is unavailable */
	int         qspi_id;       /* QSPI unit index, or -1 when not on QSPI */
} vfs_dev_t;

static const vfs_dev_t s_vfs_devs[] = {
	{ "flash", VFS_INTERNAL_FLASH_PATITION_0, LFS_FLASH,         0,                 0,                VFS_DEV_FAT_FLASH, -1 },
	{ "spi0",  VFS_SPI_FLASH_0_PATITION_0,    VFS_DEV_LFS_SPI0,  spi_flash_start,   spi_flash_size,   VFS_DEV_FAT_SPI0,  -1 },
	{ "spi1",  VFS_SPI_FLASH_1_PATITION_0,    VFS_DEV_LFS_SPI1,  spi_flash_start,   spi_flash_size,   VFS_DEV_FAT_SPI1,  -1 },
	{ "qspi0", VFS_QSPI_FLASH_0_PATITION_0,   VFS_DEV_LFS_QSPI0, qspi_lfs_p0_start, qspi_lfs_p0_size, VFS_DEV_FAT_QSPI0, 0  },
	{ "qspi1", VFS_QSPI_FLASH_1_PATITION_0,   VFS_DEV_LFS_QSPI1, qspi_lfs_p0_start, qspi_lfs_p0_size, VFS_DEV_FAT_QSPI1, VFS_DEV_QSPI1_ID },
	{ "sd",    VFS_SD_0_PATITION_0,           VFS_DEV_NO_LFS,    0,                 0,                VFS_DEV_FAT_SD,    -1 },
};

#define VFS_DEV_CNT (sizeof(s_vfs_devs) / sizeof(s_vfs_devs[0]))

static void vfs_dev_list(void)
{
	uint32_t i;

	CLI_LOGI("devices (littlefs / fatfs availability in this build):\r\n");
	for (i = 0; i < VFS_DEV_CNT; i++) {
		CLI_LOGI("  %-6s %-5s  lfs:%s fatfs:%s\r\n",
		         s_vfs_devs[i].name, s_vfs_devs[i].mount_point,
		         (s_vfs_devs[i].lfs_part_type == VFS_DEV_NO_LFS) ? "no " : "yes",
		         s_vfs_devs[i].fatfs_dev ? "yes" : "no ");
	}
}

/* Lookup without complaining: used where a non-device token is legitimate,
 * e.g. the legacy `vfs mount qspi_lfs ...` spelling. */
static const vfs_dev_t *vfs_dev_find_quiet(const char *name)
{
	uint32_t i;

	for (i = 0; i < VFS_DEV_CNT; i++) {
		if (os_strcmp(name, s_vfs_devs[i].name) == 0) {
			return &s_vfs_devs[i];
		}
	}
	return NULL;
}

static const vfs_dev_t *vfs_dev_find(const char *name)
{
	const vfs_dev_t *dev = vfs_dev_find_quiet(name);

	if (!dev) {
		CLI_LOGE("unknown device '%s'\r\n", name);
		vfs_dev_list();
	}
	return dev;
}

/* Tell the caller why a device/filesystem pair cannot be used, so a missing
 * Kconfig option does not look like a runtime failure. */
static bool vfs_dev_supports(const vfs_dev_t *dev, vfs_bench_fs_t fs)
{
	if (fs == VFS_BENCH_FS_LFS) {
		if (dev->lfs_part_type == VFS_DEV_NO_LFS) {
			CLI_LOGE("%s: littlefs not available in this build\r\n", dev->name);
			return false;
		}
	} else {
		if (!dev->fatfs_dev) {
			CLI_LOGE("%s: fatfs not available in this build "
			         "(enable the matching CONFIG_FATFS_* option)\r\n", dev->name);
			return false;
		}
	}
	return true;
}

/* Fill in the littleFS partition for a device. The internal flash carries no
 * fixed geometry here (size 0): it comes from the flash partition table. */
static int vfs_dev_lfs_part(const vfs_dev_t *dev, struct bk_little_fs_partition *part,
                            const char *mount_point)
{
	uint32_t start = dev->lfs_start;
	uint32_t size = dev->lfs_size;

	if (!size) {
		bk_logic_partition_t *fs_partition = bk_flash_partition_get_info(BK_PARTITION_FS_ID);

		if (!fs_partition) {
			CLI_LOGE("%s: no littlefs partition in the partition table\r\n", dev->name);
			return -1;
		}
		start = fs_partition->partition_start_addr;
		size = fs_partition->partition_length;
	}

	part->part_type = dev->lfs_part_type;
	part->part_flash.start_addr = start;
	part->part_flash.size = size;
	part->mount_path = mount_point;
	return 0;
}

static int vfs_dev_format(const vfs_dev_t *dev, vfs_bench_fs_t fs)
{
	if (!vfs_dev_supports(dev, fs)) {
		return -1;
	}

	if (fs == VFS_BENCH_FS_LFS) {
		struct bk_little_fs_partition part;

		if (vfs_dev_lfs_part(dev, &part, dev->mount_point)) {
			return -1;
		}
		return mkfs("PART_NONE", "littlefs", &part);
	} else {
		struct bk_fatfs_partition part;

		part.part_type = FATFS_DEVICE;
		part.mount_path = dev->mount_point;
		part.part_dev.device_name = dev->fatfs_dev;
		return mkfs("PART_NONE", "fatfs", &part);
	}
}

static int vfs_dev_mount(const vfs_dev_t *dev, vfs_bench_fs_t fs, const char *mount_point)
{
	if (!vfs_dev_supports(dev, fs)) {
		return -1;
	}

	if (!mount_point) {
		mount_point = dev->mount_point;
	}

	if (fs == VFS_BENCH_FS_LFS) {
		struct bk_little_fs_partition part;

		if (vfs_dev_lfs_part(dev, &part, mount_point)) {
			return -1;
		}
		return mount("SOURCE_NONE", mount_point, "littlefs", 0, &part);
	} else {
		struct bk_fatfs_partition part;

		part.part_type = FATFS_DEVICE;
		part.mount_path = mount_point;
		part.part_dev.device_name = dev->fatfs_dev;
		return mount("SOURCE_NONE", mount_point, "fatfs", 0, &part);
	}
}

static int vfs_fs_from_name(const char *name, vfs_bench_fs_t *out)
{
	if (os_strcmp(name, "lfs") == 0 || os_strcmp(name, "littlefs") == 0) {
		*out = VFS_BENCH_FS_LFS;
		return 0;
	}
	if (os_strcmp(name, "fatfs") == 0) {
		*out = VFS_BENCH_FS_FATFS;
		return 0;
	}
	return -1;
}

typedef struct {
	uint64_t write_bytes;
	uint64_t read_bytes;
	uint64_t write_us;
	uint64_t read_us;
	uint32_t rounds;
	uint32_t round_pass;
	uint32_t round_fail;
	uint32_t io_errors;
	uint32_t crc_errors;
	uint32_t mismatch_bytes;
	uint32_t first_bad_off;
	bool     first_bad_valid;
} vfs_bench_stat_t;

static const char *vfs_bench_fs_name(vfs_bench_fs_t fs)
{
	return (fs == VFS_BENCH_FS_FATFS) ? "fatfs" : "littlefs";
}

/*
 * Linear congruential generator (glibc constants).
 *
 * A pseudo-random payload rather than a constant or ramp byte: a fixed pattern
 * cannot expose address-aliasing faults (every page looks alike, so a
 * mis-addressed read still compares equal) and it biases the bit distribution
 * the NAND ECC engine sees. The seed changes every round so the filesystem
 * cannot collapse repeated identical writes.
 */
static inline uint32_t vfs_bench_lcg(uint32_t *s)
{
	*s = (*s * 1103515245u) + 12345u;
	return *s;
}

static void vfs_bench_fill(uint8_t *buf, uint32_t len, uint32_t *seed)
{
	uint32_t i;

	for (i = 0; i < len; i++) {
		buf[i] = (uint8_t)(vfs_bench_lcg(seed) >> 24);
	}
}

static void vfs_bench_feed_wdt(void)
{
#if CONFIG_TASK_WDT
	bk_task_wdt_feed();
#endif
}

/* KB/s with one decimal, computed in integer math: the CLI printf backend
 * cannot be relied on for %f. */
static void vfs_bench_kbps(uint64_t bytes, uint64_t us, uint32_t *whole, uint32_t *tenth)
{
	uint64_t v;

	if (us == 0) {
		*whole = 0;
		*tenth = 0;
		return;
	}

	v = (bytes * 10000000ull) / us / 1024ull;
	*whole = (uint32_t)(v / 10ull);
	*tenth = (uint32_t)(v % 10ull);
}

static void vfs_bench_stat_reset(vfs_bench_stat_t *st)
{
	os_memset(st, 0, sizeof(*st));
}

/*
 * Deliberately keyed on round_fail rather than (rounds != round_pass): `rounds`
 * is bumped when a round starts and `round_pass` only when it ends, so a status
 * query landing mid-round would otherwise report a phantom failure on a run
 * that is perfectly healthy.
 */
static bool vfs_bench_stat_failed(const vfs_bench_stat_t *st)
{
	return (st->io_errors || st->crc_errors || st->mismatch_bytes ||
	        st->round_fail != 0);
}

static void vfs_bench_report(const char *title, const char *path,
                             uint32_t file_kb, const vfs_bench_stat_t *st)
{
	uint32_t w_kbps, w_tenth, r_kbps, r_tenth;

	vfs_bench_kbps(st->write_bytes, st->write_us, &w_kbps, &w_tenth);
	vfs_bench_kbps(st->read_bytes, st->read_us, &r_kbps, &r_tenth);

	CLI_LOGI("=== %s: %s ===\r\n", title, path);
	/* rounds counts starts, so rounds > pass+fail simply means one round is
	 * still in flight when the snapshot was taken. */
	CLI_LOGI("  file=%uKB rounds=%u pass=%u fail=%u%s\r\n",
	         file_kb, st->rounds, st->round_pass, st->round_fail,
	         (st->rounds > st->round_pass + st->round_fail) ? " (1 in flight)" : "");
	CLI_LOGI("  Write: %llu B in %llu us, avg %u.%u KB/s\r\n",
	         (unsigned long long)st->write_bytes, (unsigned long long)st->write_us,
	         w_kbps, w_tenth);
	CLI_LOGI("  Read : %llu B in %llu us, avg %u.%u KB/s\r\n",
	         (unsigned long long)st->read_bytes, (unsigned long long)st->read_us,
	         r_kbps, r_tenth);
	if (st->first_bad_valid) {
		CLI_LOGI("  Verify: crc_err=%u bad_bytes=%u first_bad_off=%u\r\n",
		         st->crc_errors, st->mismatch_bytes, st->first_bad_off);
	} else {
		CLI_LOGI("  Verify: crc_err=%u bad_bytes=0 first_bad_off=-\r\n", st->crc_errors);
	}
#if CONFIG_QSPI_NAND_FLASH
	{
		uint32_t ecc_corr = 0, ecc_unc = 0;

		bk_qspi_flash_nand_get_ecc_stat(&ecc_corr, &ecc_unc);
		CLI_LOGI("  IO errors: %u   ECC corrected: %u  uncorrectable: %u\r\n",
		         st->io_errors, ecc_corr, ecc_unc);
	}
#else
	CLI_LOGI("  IO errors: %u\r\n", st->io_errors);
#endif
}

/* Dump the bad-block bookkeeping of whichever translation layer backs this
 * filesystem. Both layers log their own totals, so no new driver API needed. */
static void vfs_bench_dump_media(const vfs_dev_t *dev, vfs_bench_fs_t fs)
{
#if CONFIG_QSPI_NAND_FLASH
	/* Only the QSPI rows sit on SPI-NAND, and both translation layers are
	 * indexed per QSPI unit. Other media have no bad blocks to report. */
	if (dev->qspi_id >= 0) {
		qspi_id_t id = (qspi_id_t)(QSPI_ID_0 + dev->qspi_id);

		if (fs == VFS_BENCH_FS_LFS) {
			bk_qspi_nand_bbm_dump(id);
		}
#if CONFIG_DHARA_FTL
		else {
			bk_nand_ftl_dump(id);
		}
#endif
	}
#endif
	(void)dev;
	(void)fs;
}

/*
 * One write-then-verify pass over `path`.
 *
 * Only the write()/read() calls are timed; pattern generation and comparison
 * are excluded so the throughput figure describes the storage stack and not
 * the CPU doing the bookkeeping.
 *
 * Returns 0 when the round passed, -1 otherwise. `st` is updated either way.
 * Call it through vfs_bench_round(), which owns the failure accounting.
 */
static int vfs_bench_round_once(const char *path, uint32_t file_bytes, uint32_t seed,
                                uint8_t *wbuf, uint8_t *rbuf, vfs_bench_stat_t *st)
{
	CRC32_Context w_ctx, r_ctx;
	uint32_t w_crc = 0, r_crc = 0;
	uint32_t gen_seed = seed;
	uint32_t left, chunk;
	uint32_t off = 0;
	uint64_t t0;
	int fd;
	int ret;
	int failed = 0;

	st->rounds++;

	CRC32_Init(&w_ctx);
	CRC32_Init(&r_ctx);

	/* Start from a known-empty file on both filesystems: O_TRUNC maps to
	 * conflicting FatFs mode bits when combined with O_CREAT, so unlink
	 * first and let O_CREAT do the truncation. */
	(void)unlink(path);

	fd = open(path, O_RDWR | O_CREAT);
	if (fd < 0) {
		CLI_LOGE("bench: open '%s' for write failed (%d)\r\n", path, fd);
		st->io_errors++;
		return -1;
	}

	left = file_bytes;
	while (left) {
		chunk = (left > VFS_BENCH_CHUNK) ? VFS_BENCH_CHUNK : left;
		vfs_bench_fill(wbuf, chunk, &gen_seed);
		CRC32_Update(&w_ctx, wbuf, chunk);

		t0 = bk_aon_rtc_get_us();
		ret = write(fd, wbuf, chunk);
		st->write_us += bk_aon_rtc_get_us() - t0;

		if (ret <= 0) {
			CLI_LOGE("bench: write failed at off=%u (%d)\r\n", off, ret);
			st->io_errors++;
			failed = 1;
			break;
		}

		st->write_bytes += (uint32_t)ret;
		off += (uint32_t)ret;
		left -= (uint32_t)ret;
		vfs_bench_feed_wdt();
	}

	close(fd);

	if (failed) {
		return -1;
	}

	CRC32_Final(&w_ctx, &w_crc);

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		CLI_LOGE("bench: open '%s' for read failed (%d)\r\n", path, fd);
		st->io_errors++;
		return -1;
	}

	/* Regenerate the payload from the same seed rather than keeping the whole
	 * file in RAM: the write pass consumed the LCG sequentially, so replaying
	 * it from `seed` reproduces the expected bytes chunk by chunk. */
	gen_seed = seed;
	left = file_bytes;
	off = 0;
	while (left) {
		uint32_t i;

		chunk = (left > VFS_BENCH_CHUNK) ? VFS_BENCH_CHUNK : left;

		t0 = bk_aon_rtc_get_us();
		ret = read(fd, rbuf, chunk);
		st->read_us += bk_aon_rtc_get_us() - t0;

		if (ret <= 0) {
			CLI_LOGE("bench: read failed at off=%u (%d)\r\n", off, ret);
			st->io_errors++;
			failed = 1;
			break;
		}

		chunk = (uint32_t)ret;
		vfs_bench_fill(wbuf, chunk, &gen_seed);
		CRC32_Update(&r_ctx, rbuf, chunk);

		for (i = 0; i < chunk; i++) {
			if (rbuf[i] != wbuf[i]) {
				if (!st->first_bad_valid) {
					st->first_bad_valid = true;
					st->first_bad_off = off + i;
					CLI_LOGE("bench: mismatch at off=%u exp=0x%02x got=0x%02x\r\n",
					         off + i, wbuf[i], rbuf[i]);
				}
				st->mismatch_bytes++;
				failed = 1;
			}
		}

		st->read_bytes += chunk;
		off += chunk;
		left -= chunk;
		vfs_bench_feed_wdt();
	}

	close(fd);

	if (failed) {
		return -1;
	}

	CRC32_Final(&r_ctx, &r_crc);
	if (w_crc != r_crc) {
		/* Byte compare above found nothing, so a CRC split means the length
		 * or chunk boundaries moved, not the content. */
		CLI_LOGE("bench: crc mismatch w=0x%08x r=0x%08x\r\n", w_crc, r_crc);
		st->crc_errors++;
		return -1;
	}

	st->round_pass++;
	return 0;
}

/* Single place that books a failed round, so no early return inside
 * vfs_bench_round_once() can escape the accounting. */
static int vfs_bench_round(const char *path, uint32_t file_bytes, uint32_t seed,
                           uint8_t *wbuf, uint8_t *rbuf, vfs_bench_stat_t *st)
{
	int ret = vfs_bench_round_once(path, file_bytes, seed, wbuf, rbuf, st);

	if (ret) {
		st->round_fail++;
	}
	return ret;
}

/* Bring up one filesystem on a device. Destructive: always mkfs. */
static int vfs_bench_setup(const vfs_dev_t *dev, vfs_bench_fs_t fs)
{
	int ret;

	if (!vfs_dev_supports(dev, fs)) {
		return -1;
	}

	/* A previous run (or a manual `vfs mount`) may still hold the path. */
	(void)test_umount_vfs((char *)dev->mount_point);

	ret = vfs_dev_format(dev, fs);
	if (ret) {
		CLI_LOGE("bench: mkfs %s on %s failed (%d)\r\n",
		         vfs_bench_fs_name(fs), dev->name, ret);
		return ret;
	}

	ret = vfs_dev_mount(dev, fs, dev->mount_point);
	if (ret) {
		CLI_LOGE("bench: mount %s on %s failed (%d)\r\n",
		         vfs_bench_fs_name(fs), dev->name, ret);
		return ret;
	}

	return 0;
}

static void vfs_bench_build_path(char *out, uint32_t out_len, const char *mount_point,
                                 const char *name)
{
	snprintf(out, out_len, "%s/%s", mount_point, name);
	out[out_len - 1] = '\0';
}

/*
 * Shrink the per-round file size to something the mounted volume can hold.
 *
 * The same command has to work on media of very different sizes: the QSPI0
 * littleFS partition is 1MB on NAND but 256KB on NOR (see qspi_lfs_p0_size),
 * while FatFs-on-NOR sees the whole disk. Asking the volume avoids carrying a
 * second set of per-medium constants that would drift from the partition
 * defines. @p share_count is the number of files written concurrently, which
 * all compete for the same free space.
 *
 * Returns the size to use; 0 when the volume is too small to test at all.
 */
static uint32_t vfs_bench_fit_file_bytes(const char *mount_point, uint32_t want_bytes,
                                         uint32_t share_count)
{
	struct statfs sfs;
	uint64_t avail;
	uint32_t safe;

	if (!share_count) {
		share_count = 1;
	}

	if (statfs(mount_point, &sfs) || !sfs.f_bsize) {
		/* No usable geometry: keep the request and let the IO path report. */
		return want_bytes;
	}

	avail = (uint64_t)sfs.f_bavail * (uint64_t)sfs.f_bsize;
	avail /= share_count;

	/* Keep a quarter in reserve: both filesystems spend part of the volume on
	 * metadata that grows with file length (littleFS CTZ pointers, FAT chains),
	 * so the raw free count overstates what one file can occupy. */
	safe = (uint32_t)(avail - (avail / 4));
	safe &= ~(VFS_BENCH_CHUNK - 1U);

	if (!safe) {
		CLI_LOGE("bench: volume %s too small (avail %u B)\r\n",
		         mount_point, (uint32_t)avail);
		return 0;
	}

	if (want_bytes > safe) {
		CLI_LOGW("bench: %uKB per file exceeds %s capacity, using %uKB\r\n",
		         want_bytes / 1024U, mount_point, safe / 1024U);
		return safe;
	}

	return want_bytes;
}

static void vfs_bench_run_fs(const vfs_dev_t *dev, vfs_bench_fs_t fs,
                             uint32_t file_kb, uint32_t rounds)
{
	vfs_bench_stat_t st;
	char path[VFS_FILE_MAX_LEN + 16];
	char title[32];
	uint8_t *wbuf = NULL;
	uint8_t *rbuf = NULL;
	uint32_t file_bytes = file_kb * 1024U;
	uint32_t i;

	if (vfs_bench_setup(dev, fs)) {
		return;
	}

	wbuf = (uint8_t *)os_malloc(VFS_BENCH_CHUNK);
	rbuf = (uint8_t *)os_malloc(VFS_BENCH_CHUNK);
	if (!wbuf || !rbuf) {
		CLI_LOGE("bench: no mem for %u B buffers\r\n", VFS_BENCH_CHUNK * 2);
		goto out;
	}

	file_bytes = vfs_bench_fit_file_bytes(dev->mount_point, file_bytes, 1);
	if (!file_bytes) {
		goto out;
	}
	file_kb = file_bytes / 1024U;

	vfs_bench_stat_reset(&st);
	vfs_bench_build_path(path, sizeof(path), dev->mount_point, "bench.bin");
	snprintf(title, sizeof(title), "%s on %s", vfs_bench_fs_name(fs), dev->name);

	for (i = 0; i < rounds; i++) {
		/* Fresh seed per round; +1 keeps it non-zero for round 0. */
		if (vfs_bench_round(path, file_bytes, i + 1u, wbuf, rbuf, &st)) {
			CLI_LOGE("bench: %s round %u FAILED\r\n", title, i);
		}
	}

	(void)unlink(path);

	vfs_bench_report("VFS BENCH", title, file_kb, &st);
	vfs_bench_dump_media(dev, fs);
	CLI_LOGI("=== RESULT: %s (%s) ===\r\n",
	         vfs_bench_stat_failed(&st) ? "FAIL" : "PASS", title);

out:
	if (wbuf) {
		os_free(wbuf);
	}
	if (rbuf) {
		os_free(rbuf);
	}
	(void)test_umount_vfs((char *)dev->mount_point);
}

static void vfs_bench_cmd(int argc, char **argv)
{
	const vfs_dev_t *dev;
	uint32_t file_kb = VFS_BENCH_DEF_FILE_KB;
	uint32_t rounds = VFS_BENCH_DEF_ROUNDS;
	bool run_lfs = true;
	bool run_fatfs = true;

	if (argc < 3) {
		CLI_LOGE("usage: vfs bench <dev> [lfs|fatfs|all] [file_kb] [rounds]\r\n");
		vfs_dev_list();
		return;
	}

	dev = vfs_dev_find(argv[2]);
	if (!dev) {
		return;
	}

	if (argc >= 4) {
		if (os_strcmp(argv[3], "all") == 0) {
			/* keep both */
		} else if (os_strcmp(argv[3], "lfs") == 0 || os_strcmp(argv[3], "littlefs") == 0) {
			run_fatfs = false;
		} else if (os_strcmp(argv[3], "fatfs") == 0) {
			run_lfs = false;
		} else {
			CLI_LOGE("usage: vfs bench <dev> [lfs|fatfs|all] [file_kb] [rounds]\r\n");
			return;
		}
	}

	if (argc >= 5) {
		file_kb = os_strtoul(argv[4], NULL, 10);
	}
	if (argc >= 6) {
		rounds = os_strtoul(argv[5], NULL, 10);
	}

	if (!file_kb || file_kb > VFS_BENCH_MAX_FILE_KB) {
		CLI_LOGE("bench: file_kb must be 1..%u\r\n", VFS_BENCH_MAX_FILE_KB);
		return;
	}
	if (!rounds) {
		CLI_LOGE("bench: rounds must be >= 1\r\n");
		return;
	}

	/* `all` on a device that only supports one of the two is not an error;
	 * run what is available and let vfs_dev_supports() explain the rest. */
	if (run_lfs && run_fatfs) {
		if (dev->lfs_part_type == VFS_DEV_NO_LFS || !dev->fatfs_dev) {
			run_lfs = (dev->lfs_part_type != VFS_DEV_NO_LFS);
			run_fatfs = !!dev->fatfs_dev;
			if (!run_lfs && !run_fatfs) {
				CLI_LOGE("%s: neither littlefs nor fatfs available in this build\r\n",
				         dev->name);
				return;
			}
		}
	}

#if CONFIG_QSPI_NAND_FLASH
	/*
	 * Refuse up front rather than dying inside the second mkfs.
	 *
	 * On SPI-NAND the two filesystems cannot share a chip. littleFS sits on the
	 * BBM, which always claims the whole device (bad-block table in the top two
	 * blocks, spare pool directly below), while FatFs sits on the Dhara FTL over
	 * CONFIG_NAND_FTL_START_BLOCK/COUNT - so no start-block value separates
	 * them. Worse, whichever runs first leaves the chip unusable for the other:
	 * pages programmed with on-die ECC enabled no longer read back 0xFF in
	 * spare[0], which is the byte the ECC-off factory-bad check samples, so
	 * every already-written block looks factory-bad. Dhara then burns through
	 * its DHARA_MAX_RETRIES (8) block retries and mkfs fails with
	 * "Too many bad blocks".
	 */
	if (run_lfs && run_fatfs && dev->qspi_id >= 0) {
		CLI_LOGE("bench: %s is SPI-NAND; lfs and fatfs cannot share one chip\r\n", dev->name);
		CLI_LOGE("  littlefs owns the whole chip via the BBM, fatfs via the Dhara FTL;\r\n");
		CLI_LOGE("  blocks written by one read back as factory-bad to the other\r\n");
		CLI_LOGE("  run one at a time: vfs bench %s lfs   /   vfs bench %s fatfs\r\n",
		         dev->name, dev->name);
		CLI_LOGE("  to hand the chip over, raw-erase the used blocks first "
		         "(qspi_nand block_erase <blk>)\r\n");
		return;
	}
#endif

	CLI_LOGW("bench: DESTRUCTIVE - reformats %s at %s\r\n", dev->name, dev->mount_point);

	if (run_lfs) {
		vfs_bench_run_fs(dev, VFS_BENCH_FS_LFS, file_kb, rounds);
	}
	if (run_fatfs) {
		vfs_bench_run_fs(dev, VFS_BENCH_FS_FATFS, file_kb, rounds);
	}
}

static beken_thread_t s_soak_handle = NULL;
static volatile bool  s_soak_stop = false;
static volatile bool  s_soak_running = false;
static vfs_bench_stat_t s_soak_stat;
static vfs_bench_fs_t s_soak_fs = VFS_BENCH_FS_LFS;
static const vfs_dev_t *s_soak_dev;
static uint32_t s_soak_file_kb = VFS_BENCH_DEF_FILE_KB;

static void vfs_soak_task(beken_thread_arg_t arg)
{
	const vfs_dev_t *dev = s_soak_dev;
	char path[VFS_FILE_MAX_LEN + 16];
	uint8_t *wbuf = NULL;
	uint8_t *rbuf = NULL;
	uint32_t file_bytes = s_soak_file_kb * 1024U;
	uint32_t seed = 1;

	(void)arg;

	wbuf = (uint8_t *)os_malloc(VFS_BENCH_CHUNK);
	rbuf = (uint8_t *)os_malloc(VFS_BENCH_CHUNK);
	if (!wbuf || !rbuf) {
		CLI_LOGE("soak: no mem\r\n");
		goto out;
	}

	if (vfs_bench_setup(dev, s_soak_fs)) {
		goto out;
	}

	file_bytes = vfs_bench_fit_file_bytes(dev->mount_point, file_bytes, 1);
	if (!file_bytes) {
		goto out;
	}
	s_soak_file_kb = file_bytes / 1024U;

	vfs_bench_build_path(path, sizeof(path), dev->mount_point, "soak.bin");
	CLI_LOGI("soak: started dev=%s fs=%s file=%uKB path=%s\r\n",
	         dev->name, vfs_bench_fs_name(s_soak_fs), s_soak_file_kb, path);

	while (!s_soak_stop) {
		if (vfs_bench_round(path, file_bytes, seed, wbuf, rbuf, &s_soak_stat)) {
			CLI_LOGE("soak: round %u FAILED (errors io=%u crc=%u bad=%u)\r\n",
			         s_soak_stat.rounds, s_soak_stat.io_errors,
			         s_soak_stat.crc_errors, s_soak_stat.mismatch_bytes);
		}

		seed++;
		if (seed == 0) {
			seed = 1;
		}

		if ((s_soak_stat.rounds % VFS_SOAK_REPORT_ROUNDS) == 0) {
			vfs_bench_report("VFS SOAK", vfs_bench_fs_name(s_soak_fs),
			                 s_soak_file_kb, &s_soak_stat);
		}

		vfs_bench_feed_wdt();
		rtos_delay_milliseconds(VFS_SOAK_IDLE_MS);
	}

	(void)unlink(path);
	vfs_bench_report("VFS SOAK FINAL", vfs_bench_fs_name(s_soak_fs),
	                 s_soak_file_kb, &s_soak_stat);
	vfs_bench_dump_media(dev, s_soak_fs);
	(void)test_umount_vfs((char *)dev->mount_point);

out:
	if (wbuf) {
		os_free(wbuf);
	}
	if (rbuf) {
		os_free(rbuf);
	}

	s_soak_running = false;
	s_soak_handle = NULL;
	rtos_delete_thread(NULL);
}

static void vfs_soak_cmd(int argc, char **argv)
{
	if (argc < 3) {
		CLI_LOGE("usage: vfs soak start <dev> [lfs|fatfs] [file_kb]\r\n");
		CLI_LOGE("       vfs soak stop|status\r\n");
		vfs_dev_list();
		return;
	}

	if (os_strcmp(argv[2], "start") == 0) {
		const vfs_dev_t *dev;

		if (s_soak_running) {
			CLI_LOGW("soak: already running\r\n");
			return;
		}

		if (argc < 4) {
			CLI_LOGE("usage: vfs soak start <dev> [lfs|fatfs] [file_kb]\r\n");
			vfs_dev_list();
			return;
		}

		dev = vfs_dev_find(argv[3]);
		if (!dev) {
			return;
		}

		s_soak_fs = VFS_BENCH_FS_LFS;
		s_soak_file_kb = VFS_BENCH_DEF_FILE_KB;

		if (argc >= 5) {
			if (vfs_fs_from_name(argv[4], &s_soak_fs)) {
				CLI_LOGE("soak: fs must be lfs or fatfs\r\n");
				return;
			}
		}
		if (argc >= 6) {
			s_soak_file_kb = os_strtoul(argv[5], NULL, 10);
		}
		if (!s_soak_file_kb || s_soak_file_kb > VFS_BENCH_MAX_FILE_KB) {
			CLI_LOGE("soak: file_kb must be 1..%u\r\n", VFS_BENCH_MAX_FILE_KB);
			return;
		}
		if (!vfs_dev_supports(dev, s_soak_fs)) {
			return;
		}

		s_soak_dev = dev;
		vfs_bench_stat_reset(&s_soak_stat);
		s_soak_stop = false;
		s_soak_running = true;

		CLI_LOGW("soak: DESTRUCTIVE - reformats %s at %s\r\n", dev->name, dev->mount_point);
		if (rtos_create_thread(&s_soak_handle, 5, "vfs_soak",
		                       (beken_thread_function_t)vfs_soak_task, 4 * 1024, 0)) {
			CLI_LOGE("soak: create thread failed\r\n");
			s_soak_running = false;
			s_soak_handle = NULL;
		}
	} else if (os_strcmp(argv[2], "stop") == 0) {
		if (!s_soak_running) {
			CLI_LOGW("soak: not running\r\n");
			return;
		}
		s_soak_stop = true;
		CLI_LOGI("soak: stop requested, finishing current round\r\n");
	} else if (os_strcmp(argv[2], "status") == 0) {
		CLI_LOGI("soak: %s dev=%s\r\n", s_soak_running ? "RUNNING" : "STOPPED",
		         s_soak_dev ? s_soak_dev->name : "-");
		vfs_bench_report("VFS SOAK STATUS", vfs_bench_fs_name(s_soak_fs),
		                 s_soak_file_kb, &s_soak_stat);
	} else {
		CLI_LOGE("usage: vfs soak start <dev> [lfs|fatfs] [file_kb]\r\n");
		CLI_LOGE("       vfs soak stop|status\r\n");
	}
}

static beken_thread_t   s_ts_handle[VFS_TS_MAX_TASKS];
static uint8_t         *s_ts_wbuf[VFS_TS_MAX_TASKS];
static uint8_t         *s_ts_rbuf[VFS_TS_MAX_TASKS];
static vfs_bench_stat_t s_ts_stat[VFS_TS_MAX_TASKS];
static char             s_ts_path[VFS_TS_MAX_TASKS][VFS_FILE_MAX_LEN + 16];
static volatile bool    s_ts_stop = false;
static uint32_t         s_ts_file_kb = VFS_TS_DEF_FILE_KB;

/* Derived from the handle slots rather than kept in a counter: each task owns
 * exactly one slot, so there is no shared value for concurrent exits to race
 * on and lose an update (which would wedge the "still running" check). */
static uint32_t vfs_ts_running_count(void)
{
	uint32_t i, n = 0;

	for (i = 0; i < VFS_TS_MAX_TASKS; i++) {
		if (s_ts_handle[i]) {
			n++;
		}
	}
	return n;
}

static void vfs_ts_free_slot(uint32_t id)
{
	if (s_ts_wbuf[id]) {
		os_free(s_ts_wbuf[id]);
		s_ts_wbuf[id] = NULL;
	}
	if (s_ts_rbuf[id]) {
		os_free(s_ts_rbuf[id]);
		s_ts_rbuf[id] = NULL;
	}
}

static void vfs_ts_task(beken_thread_arg_t arg)
{
	uint32_t id = ((uint32_t)arg) % VFS_TS_MAX_TASKS;
	uint32_t file_bytes = s_ts_file_kb * 1024U;
	uint32_t seed = id + 1u;

	CLI_LOGI("ts[%u]: start path=%s\r\n", id, s_ts_path[id]);

	while (!s_ts_stop) {
		if (vfs_bench_round(s_ts_path[id], file_bytes, seed,
		                    s_ts_wbuf[id], s_ts_rbuf[id], &s_ts_stat[id])) {
			CLI_LOGE("ts[%u]: round %u FAILED (io=%u crc=%u bad=%u)\r\n",
			         id, s_ts_stat[id].rounds, s_ts_stat[id].io_errors,
			         s_ts_stat[id].crc_errors, s_ts_stat[id].mismatch_bytes);
		}

		seed += VFS_TS_MAX_TASKS;
		if (seed == 0) {
			seed = id + 1u;
		}

		vfs_bench_feed_wdt();
		/* Let the peers in, otherwise a high-priority task starves the rest
		 * and the run stops being a concurrency test. */
		rtos_thread_msleep(20);
	}

	(void)unlink(s_ts_path[id]);
	CLI_LOGI("ts[%u]: stopped rounds=%u pass=%u fail=%u io=%u crc=%u bad=%u\r\n",
	         id, s_ts_stat[id].rounds, s_ts_stat[id].round_pass,
	         s_ts_stat[id].round_fail, s_ts_stat[id].io_errors,
	         s_ts_stat[id].crc_errors, s_ts_stat[id].mismatch_bytes);

	vfs_ts_free_slot(id);
	s_ts_handle[id] = NULL;
	rtos_delete_thread(NULL);
}

static void vfs_ts_status(void)
{
	uint32_t i;

	CLI_LOGI("threadsafe: %u task(s) active, file=%uKB\r\n",
	         vfs_ts_running_count(), s_ts_file_kb);
	for (i = 0; i < VFS_TS_MAX_TASKS; i++) {
		if (s_ts_stat[i].rounds == 0 && s_ts_handle[i] == NULL) {
			continue;
		}
		CLI_LOGI("  ts[%u] %s rounds=%u pass=%u fail=%u io=%u crc=%u bad=%u\r\n",
		         i, s_ts_handle[i] ? "RUN " : "DONE",
		         s_ts_stat[i].rounds, s_ts_stat[i].round_pass,
		         s_ts_stat[i].round_fail, s_ts_stat[i].io_errors,
		         s_ts_stat[i].crc_errors, s_ts_stat[i].mismatch_bytes);
	}
}

static void vfs_threadsafe_cmd(int argc, char **argv)
{
	const vfs_dev_t *def_dev;
	uint32_t task_count;
	uint32_t file_bytes;
	uint32_t started = 0;
	uint32_t i;
	const char *mount_point;

	if (argc < 3) {
		CLI_LOGE("usage: vfs threadsafe_test <tasks> [file_kb] [mount_point]\r\n");
		CLI_LOGE("       vfs threadsafe_test stop|status\r\n");
		CLI_LOGE("note: the volume must already be mounted (vfs mount ...)\r\n");
		return;
	}

	def_dev = vfs_dev_find(VFS_BENCH_DEF_DEV);
	mount_point = def_dev ? def_dev->mount_point : VFS_QSPI_FLASH_0_PATITION_0;

	if (os_strcmp(argv[2], "stop") == 0) {
		if (!vfs_ts_running_count()) {
			CLI_LOGW("threadsafe: not running\r\n");
			return;
		}
		s_ts_stop = true;
		CLI_LOGI("threadsafe: stop requested\r\n");
		return;
	}

	if (os_strcmp(argv[2], "status") == 0) {
		vfs_ts_status();
		return;
	}

	if (vfs_ts_running_count()) {
		CLI_LOGW("threadsafe: %u task(s) still running, stop them first\r\n",
		         vfs_ts_running_count());
		return;
	}

	task_count = os_strtoul(argv[2], NULL, 10);
	if (!task_count || task_count > VFS_TS_MAX_TASKS) {
		CLI_LOGE("threadsafe: tasks must be 1..%u\r\n", VFS_TS_MAX_TASKS);
		return;
	}

	s_ts_file_kb = VFS_TS_DEF_FILE_KB;
	if (argc >= 4) {
		s_ts_file_kb = os_strtoul(argv[3], NULL, 10);
	}
	if (!s_ts_file_kb || s_ts_file_kb > VFS_BENCH_MAX_FILE_KB) {
		CLI_LOGE("threadsafe: file_kb must be 1..%u\r\n", VFS_BENCH_MAX_FILE_KB);
		return;
	}

	/* The path must live under a mounted point: bk_vfs_lookup() resolves by
	 * mount-point prefix, so a bare "/name.txt" matches nothing and every
	 * open() fails. */
	if (argc >= 5) {
		mount_point = argv[4];
	}

	/* Every task writes its own file to this one volume, so the per-file size
	 * has to be divided by the task count. */
	file_bytes = vfs_bench_fit_file_bytes(mount_point, s_ts_file_kb * 1024U, task_count);
	if (!file_bytes) {
		return;
	}
	s_ts_file_kb = file_bytes / 1024U;
	if (!s_ts_file_kb) {
		CLI_LOGE("threadsafe: volume %s too small for %u task(s)\r\n",
		         mount_point, task_count);
		return;
	}

	s_ts_stop = false;

	for (i = 0; i < task_count; i++) {
		vfs_bench_stat_reset(&s_ts_stat[i]);

		/* Allocate before creating the thread: the task dereferences these
		 * buffers as soon as it is scheduled. */
		s_ts_wbuf[i] = (uint8_t *)os_malloc(VFS_BENCH_CHUNK);
		s_ts_rbuf[i] = (uint8_t *)os_malloc(VFS_BENCH_CHUNK);
		if (!s_ts_wbuf[i] || !s_ts_rbuf[i]) {
			CLI_LOGE("threadsafe: no mem for task %u\r\n", i);
			vfs_ts_free_slot(i);
			break;
		}

		snprintf(s_ts_path[i], sizeof(s_ts_path[i]), "%s/ts_%u.bin", mount_point, i);
		s_ts_path[i][sizeof(s_ts_path[i]) - 1] = '\0';

		if (rtos_create_thread(&s_ts_handle[i], 5, "vfs_ts",
		                       (beken_thread_function_t)vfs_ts_task,
		                       4 * 1024, (beken_thread_arg_t)i)) {
			CLI_LOGE("threadsafe: create task %u failed\r\n", i);
			s_ts_handle[i] = NULL;
			vfs_ts_free_slot(i);
			break;
		}
		started++;
	}

	if (!started) {
		s_ts_stop = true;
		CLI_LOGE("threadsafe: no task started\r\n");
		return;
	}

	CLI_LOGI("threadsafe: %u task(s) on %s, file=%uKB; 'vfs threadsafe_test stop' to end\r\n",
	         started, mount_point, s_ts_file_kb);
}


void cli_vfs_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret;

	if (argc < 2) {
		BK_LOGD(NULL, "usage : vfs format <dev> <lfs|fatfs>\n");
		BK_LOGD(NULL, "        vfs mount <dev> <lfs|fatfs> [mount_point]\n");
		BK_LOGD(NULL, "        vfs umount|read|write|unlink|rename ...\n");
		BK_LOGD(NULL, "        vfs bench <dev> [lfs|fatfs|all] [file_kb] [rounds]\n");
		BK_LOGD(NULL, "        vfs soak start <dev> [lfs|fatfs] [file_kb] | stop | status\n");
		BK_LOGD(NULL, "        vfs threadsafe_test <tasks> [file_kb] [mount_point]\n");
		BK_LOGD(NULL, "        vfs devs\n");
		return;
	}

	if (os_strcmp(argv[1], "devs") == 0) {
		vfs_dev_list();
		return;
	}

	if (os_strcmp(argv[1], "format") == 0) {
		if (argc < 3) {
			BK_LOGD(NULL, "usage : vfs format <dev> <lfs|fatfs>\n");
			vfs_dev_list();
			return;
		}
		/* Orthogonal form `vfs format <dev> <fs>`. Legacy names below mix the
		 * medium and the filesystem into one token and never collide with a
		 * device name, so trying the device table first is unambiguous. */
		{
			const vfs_dev_t *dev = vfs_dev_find_quiet(argv[2]);

			if (dev) {
				vfs_bench_fs_t fs;

				if (argc < 4 || vfs_fs_from_name(argv[3], &fs)) {
					BK_LOGD(NULL, "usage : vfs format <dev> <lfs|fatfs>\n");
					return;
				}
				ret = vfs_dev_format(dev, fs);
				BK_LOGD(NULL, "format ret=%d\n", ret);
				return;
			}
		}

		if (os_strcmp(argv[2], "lfs") == 0)
			ret = test_format_lfs();
		else if (os_strcmp(argv[2], "spi_lfs") == 0)
			ret = test_format_spi_lfs();
		else if (os_strcmp(argv[2], "qspi_lfs") == 0)
			ret = test_format_qspi_lfs();
		else if (os_strcmp(argv[2], "qspi0_1_lfs") == 0)
			ret = test_format_qspi0_part1_lfs();
		else if (os_strcmp(argv[2], "fatfs") == 0)
			ret = test_format_fatfs();
		else if (os_strcmp(argv[2], "qspi0_fatfs") == 0)
			ret = test_format_qspi0_fatfs();
		else if (os_strcmp(argv[2], "flash_fatfs") == 0)
			ret = test_format_flash_fatfs();
		else {
			BK_LOGD(NULL, "usage : vfs format <dev> <lfs|fatfs>\n");
			vfs_dev_list();
			return;
		}
		BK_LOGD(NULL, "format ret=%d\n", ret);
	} else if (os_strcmp(argv[1], "mount") == 0) {
		char *mount_point;

		if (argc < 3) {
			BK_LOGD(NULL, "usage : vfs mount <dev> <lfs|fatfs> [MOUNT_POINT]\n");
			vfs_dev_list();
			return;
		}
		/* Orthogonal form `vfs mount <dev> <fs> [mount_point]`; the mount point
		 * defaults to the device's standard path. */
		{
			const vfs_dev_t *dev = vfs_dev_find_quiet(argv[2]);

			if (dev) {
				vfs_bench_fs_t fs;

				if (argc < 4 || vfs_fs_from_name(argv[3], &fs)) {
					BK_LOGD(NULL, "usage : vfs mount <dev> <lfs|fatfs> [MOUNT_POINT]\n");
					return;
				}
				ret = vfs_dev_mount(dev, fs, (argc >= 5) ? argv[4] : NULL);
				BK_LOGD(NULL, "mount ret=%d\n", ret);
				return;
			}
		}

		if (argc < 4) {
			BK_LOGD(NULL, "usage : vfs mount <dev> <lfs|fatfs> [MOUNT_POINT]\n");
			vfs_dev_list();
			return;
		}
		mount_point = argv[3];

		if (os_strcmp(argv[2], "lfs") == 0)
			ret = test_mount_lfs(mount_point);
		else if (os_strcmp(argv[2], "spi_lfs") == 0)
			ret = test_mount_spi_lfs(mount_point);
		else if (os_strcmp(argv[2], "qspi_lfs") == 0)
			ret = test_mount_qspi_lfs(mount_point);
		else if (os_strcmp(argv[2], "qspi0_1_lfs") == 0)
			ret = test_mount_qspi0_part1_lfs(mount_point);
		else if (os_strcmp(argv[2], "fatfs") == 0)
			ret = test_mount_fatfs(mount_point);
		else if (os_strcmp(argv[2], "qspi0_fatfs") == 0)
			ret = test_mount_qspi0_fatfs(mount_point);
		else if (os_strcmp(argv[2], "flash_fatfs") == 0)
			ret = test_mount_flash_fatfs(mount_point);
		else {
			BK_LOGD(NULL, "usage : vfs mount lfs|fatfs MOUNT_POINT\n");
			return;
		}
		BK_LOGD(NULL, "mount ret=%d\n", ret);
	} else if (os_strcmp(argv[1], "umount") == 0) {
		char *mount_point;

		if (argc < 3) {
			BK_LOGD(NULL, "usage : vfs umount MOUNT_POINT\n");
			return;
		}
		
		mount_point = argv[2];

		ret = test_umount_vfs(mount_point);
		BK_LOGD(NULL, "umount ret=%d\n", ret);
	} else if (os_strcmp(argv[1], "read") == 0) {
		char *file_name;
		
		if (argc < 3) {
			BK_LOGD(NULL, "usage : vfs read FULL_FILE_NAME\n");
			return;
		}
		file_name = argv[2];

		ret = test_read_vfs(file_name);
		BK_LOGD(NULL, "read ret=%d\n", ret);
	} else if (os_strcmp(argv[1], "write") == 0) {
		char *file_name;
		char *content;
		
		if (argc < 4) {
			BK_LOGD(NULL, "usage : vfs write FULL_FILE_NAME CONTENT\n");
			return;
		}
		file_name = argv[2];
		content = argv[3];

		ret = test_write_vfs(file_name, content);
		BK_LOGD(NULL, "write ret=%d\n", ret);
	} else if (os_strcmp(argv[1], "unlink") == 0) {
		char *file_name;
		
		if (argc < 3) {
			BK_LOGD(NULL, "usage : vfs unlink FULL_FILE_NAME\n");
			return;
		}
		file_name = argv[2];

		ret = test_unlink_vfs(file_name);
		BK_LOGD(NULL, "unlink ret=%d\n", ret);
	} else if (os_strcmp(argv[1], "rename") == 0) {
		char *old_name;
		char *new_name;

		if (argc < 4) {
			BK_LOGD(NULL, "usage : vfs rename OLD_FILE_NAME NEW_FILE_NAME\n");
			return;
		}
		old_name = argv[2];
		new_name = argv[3];

		ret = test_rename_vfs(old_name, new_name);
		BK_LOGD(NULL, "rename ret=%d\n", ret);
	} else if (os_strcmp(argv[1], "bench") == 0) {
		vfs_bench_cmd(argc, argv);
	} else if (os_strcmp(argv[1], "soak") == 0) {
		vfs_soak_cmd(argc, argv);
	} else if (os_strcmp(argv[1], "threadsafe_test") == 0) {
		vfs_threadsafe_cmd(argc, argv);
	} else {
		BK_LOGD(NULL, "vfs unknown sub cmd %s\n", argv[1]);
	}
}

#define VFS_CMD_CNT (sizeof(vfs_commands) / sizeof(struct cli_command))
static const struct cli_command vfs_commands[] = {
	{"vfs", "vfs devs|format|mount|umount|read|write|unlink|bench|soak|threadsafe_test", cli_vfs_test},
};

int cli_vfs_init(void)
{
	return cli_register_commands(vfs_commands, VFS_CMD_CNT);
}

#endif
