#include "cli.h"

#if CONFIG_DHARA_FTL

#include <os/os.h>
#include <os/mem.h>
#include <driver/nand_ftl.h>

#define NAND_FTL_CLI_QSPI_ID   QSPI_ID_0

static void nand_ftl_usage(void)
{
	CLI_LOGI("nand_ftl <cmd> [args]\r\n");
	CLI_LOGI("  init                    - bring up the FTL block device\r\n");
	CLI_LOGI("  dump                    - print geometry/capacity/bad blocks\r\n");
	CLI_LOGI("  format                  - clear the map (wipe, no FS)\r\n");
	CLI_LOGI("  sync                    - flush pending writes durable\r\n");
	CLI_LOGI("  bad <block>             - inject/retire a logical block (destructive)\r\n");
	CLI_LOGI("  wr <sector> <byte>      - write one 512B sector filled with byte\r\n");
	CLI_LOGI("  rd <sector>             - read one 512B sector (first 32 bytes)\r\n");
	CLI_LOGI("  verify <sector> <byte>  - write+readback one sector, check pattern\r\n");
	CLI_LOGI("  baseline                - scan partition for factory bad blocks\r\n");
}

static void nand_ftl_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		nand_ftl_usage();
		return;
	}

	qspi_id_t id = NAND_FTL_CLI_QSPI_ID;

	if (os_strcmp(argv[1], "init") == 0) {
		bk_err_t r = bk_nand_ftl_init(id);
		CLI_LOGI("nand_ftl init: %d (sectors=%u)\r\n", r, bk_nand_ftl_sector_count(id));
	} else if (os_strcmp(argv[1], "dump") == 0) {
		bk_nand_ftl_dump(id);
	} else if (os_strcmp(argv[1], "format") == 0) {
		CLI_LOGI("nand_ftl format: %d\r\n", bk_nand_ftl_format(id));
	} else if (os_strcmp(argv[1], "sync") == 0) {
		CLI_LOGI("nand_ftl sync: %d\r\n", bk_nand_ftl_sync(id));
	} else if (os_strcmp(argv[1], "bad") == 0 && argc >= 3) {
		uint32_t blk = os_strtoul(argv[2], NULL, 0);
		CLI_LOGI("nand_ftl inject bad %u: %d\r\n", blk, bk_nand_ftl_inject_bad(id, blk));
	} else if (os_strcmp(argv[1], "wr") == 0 && argc >= 4) {
		uint32_t sector = os_strtoul(argv[2], NULL, 0);
		uint8_t val = (uint8_t)os_strtoul(argv[3], NULL, 0);
		uint8_t *buf = (uint8_t *)os_malloc(BK_NAND_FTL_SECTOR_SIZE);
		if (!buf) { CLI_LOGE("no mem\r\n"); return; }
		os_memset(buf, val, BK_NAND_FTL_SECTOR_SIZE);
		bk_err_t r = bk_nand_ftl_write(id, sector, buf, 1);
		bk_nand_ftl_sync(id);
		os_free(buf);
		CLI_LOGI("nand_ftl wr sector=%u val=0x%02x: %d\r\n", sector, val, r);
	} else if (os_strcmp(argv[1], "rd") == 0 && argc >= 3) {
		uint32_t sector = os_strtoul(argv[2], NULL, 0);
		uint8_t *buf = (uint8_t *)os_malloc(BK_NAND_FTL_SECTOR_SIZE);
		if (!buf) { CLI_LOGE("no mem\r\n"); return; }
		bk_err_t r = bk_nand_ftl_read(id, sector, buf, 1);
		CLI_LOGI("nand_ftl rd sector=%u: %d\r\n", sector, r);
		if (r == BK_OK) {
			char line[32 * 3 + 1];
			int n = 0;
			for (int i = 0; i < 32; i++) {
				n += snprintf(line + n, sizeof(line) - n, "%02x ", buf[i]);
			}
			CLI_LOGI("%s\r\n", line);
		}
		os_free(buf);
	} else if (os_strcmp(argv[1], "verify") == 0 && argc >= 4) {
		uint32_t sector = os_strtoul(argv[2], NULL, 0);
		uint8_t val = (uint8_t)os_strtoul(argv[3], NULL, 0);
		uint8_t *wb = (uint8_t *)os_malloc(BK_NAND_FTL_SECTOR_SIZE);
		uint8_t *rb = (uint8_t *)os_malloc(BK_NAND_FTL_SECTOR_SIZE);
		if (!wb || !rb) { CLI_LOGE("no mem\r\n"); if (wb) os_free(wb); if (rb) os_free(rb); return; }
		os_memset(wb, val, BK_NAND_FTL_SECTOR_SIZE);
		bk_err_t r = bk_nand_ftl_write(id, sector, wb, 1);
		if (r == BK_OK) r = bk_nand_ftl_sync(id);
		if (r == BK_OK) r = bk_nand_ftl_read(id, sector, rb, 1);
		int mism = (r == BK_OK) ? os_memcmp(wb, rb, BK_NAND_FTL_SECTOR_SIZE) : -1;
		CLI_LOGI("nand_ftl verify sector=%u val=0x%02x: %s (r=%d)\r\n",
			 sector, val, (r == BK_OK && mism == 0) ? "PASS" : "FAIL", r);
		os_free(wb);
		os_free(rb);
	} else if (os_strcmp(argv[1], "baseline") == 0) {
		bk_nand_ftl_scan_factory_bad(id);
	} else {
		nand_ftl_usage();
	}
}

#define NAND_FTL_CMD_CNT (sizeof(s_nand_ftl_commands) / sizeof(struct cli_command))
static const struct cli_command s_nand_ftl_commands[] = {
	{"nand_ftl", "nand_ftl <init|dump|format|sync|bad|wr|rd|verify|baseline> ...", nand_ftl_cmd},
};

int cli_nand_ftl_init(void)
{
	return cli_register_commands(s_nand_ftl_commands, NAND_FTL_CMD_CNT);
}

#endif /* CONFIG_DHARA_FTL */
