#include <stdint.h>
#include <stdbool.h>
#include <os/os.h>
#include <os/str.h>
#include "cli.h"
#include "bk_private/bk_cli.h"

#define CP_KWS_ZERO_PCM_BYTES   1280u
#define CP_KWS_ZERO_PCM_MAX     4096u

static bool s_cp_kws_initialized;

static void cp_kws_help(void)
{
	os_printf("cp_kws info\r\n");
	os_printf("cp_kws init\r\n");
	os_printf("cp_kws recog_zero [bytes]\r\n");
	os_printf("cp_kws deinit\r\n");
	os_printf("cp_kws smoke\r\n");
}

static void cp_kws_release_buffers(void)
{
	s_cp_kws_initialized = false;
}

static int cp_kws_init_once(void)
{
	if (s_cp_kws_initialized) {
		os_printf("cp_kws already initialized\r\n");
		return BK_OK;
	}

	s_cp_kws_initialized = true;
	os_printf("cp_kws stub init ok: CP KWS algorithm library is not linked\r\n");
	return BK_OK;
}

static int cp_kws_recog_zero(uint32_t bytes)
{
	if (bytes == 0 || bytes > CP_KWS_ZERO_PCM_MAX || (bytes & 1u)) {
		os_printf("cp_kws invalid pcm bytes: %u\r\n", bytes);
		return BK_FAIL;
	}

	os_printf("cp_kws stub recog_zero: bytes=%u result=success\r\n", bytes);
	return BK_OK;
}

static void cli_cp_kws_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 2 || argv[1] == NULL) {
		cp_kws_help();
		return;
	}

	if (os_strcmp(argv[1], "info") == 0) {
		os_printf("cp_kws info: initialized=%d mode=stub_success lib=not_linked\r\n",
				  s_cp_kws_initialized);
	} else if (os_strcmp(argv[1], "init") == 0) {
		(void)cp_kws_init_once();
	} else if (os_strcmp(argv[1], "recog_zero") == 0) {
		uint32_t bytes = CP_KWS_ZERO_PCM_BYTES;
		if (argc > 2 && argv[2]) {
			bytes = os_strtoul(argv[2], NULL, 10);
		}
		(void)cp_kws_recog_zero(bytes);
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		cp_kws_release_buffers();
		os_printf("cp_kws deinit ok\r\n");
	} else if (os_strcmp(argv[1], "smoke") == 0) {
		if (cp_kws_init_once() == BK_OK) {
			(void)cp_kws_recog_zero(CP_KWS_ZERO_PCM_BYTES);
#if CONFIG_CP_PROMPT_TEST
			extern int cp_prompt_play_success(void);
			(void)cp_prompt_play_success();
#endif
		}
		cp_kws_release_buffers();
		os_printf("cp_kws smoke done\r\n");
	} else {
		cp_kws_help();
	}
}

static const struct cli_command s_cp_kws_commands[] = {
	{"cp_kws", "cp_kws {info|init|recog_zero|deinit|smoke}", cli_cp_kws_cmd},
};

int cli_cp_kws_init(void)
{
	return cli_register_commands(s_cp_kws_commands,
								 sizeof(s_cp_kws_commands) / sizeof(s_cp_kws_commands[0]));
}
