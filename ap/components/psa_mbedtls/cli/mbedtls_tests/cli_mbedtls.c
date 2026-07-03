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

#include "cli.h"
#include "mbedtls_test.h"
#include <os/mem.h>
#include <driver/aon_rtc.h>
#include "sys_hal.h"
#include "modules/pm.h"
#include <os/os.h>

#include "mbedtls/platform.h"

#define MBEDTLS_SHA_TEST_CNT    1
#define MBEDTLS_TEST_FREQUENCY  (PM_CPU_FRQ_240M)
#define MBEDTLS_TEST_PRIORITY   4

static void cli_mbedtls_help(void)
{
	CLI_LOGD("mbedtls_sha 256/512\r\n");
	CLI_LOGD("mbedtls_aes ecb/cbc/ctr/gcm\r\n");
	CLI_LOGD("mbedtls_ecdsa [cnt]\r\n");
	CLI_LOGD("mbedtls_rsa\r\n");
	CLI_LOGD("mbedtls_rand {basic|uniq|loop [cnt]}\r\n");
	CLI_LOGD("mbedtls_tls [cnt]\r\n");
	CLI_LOGD("mbedtls_selftest\r\n");
	CLI_LOGD("mbedtls_thread create [cnt]\r\n");
}

#define err_if(expr,status)                                          \
  do {                                                               \
    if (expr) {                                                      \
        CLI_LOGE("FAILED: %s(%d)(%d)\r\n",__func__,__LINE__,status); \
        err_cnt++;                                                   \
    }                                                                \
  } while(0)

const uint32_t test_len[] = {32, 1024, 4096};

static void mbedtls_tls_log_mem_probe(const char *stage)
{
#if defined(MBEDTLS_PLATFORM_MEMORY)
	enum { probe_len = 256 };
	size_t free_before = rtos_get_psram_free_heap_size();
	uint32_t used_before = bk_psram_heap_get_used_count();
	void *ptr = mbedtls_calloc(1, probe_len);
	size_t free_after_alloc = rtos_get_psram_free_heap_size();
	uint32_t used_after_alloc = bk_psram_heap_get_used_count();

	CLI_LOGD("MEM %s mbedtls_calloc ptr=%p len=%u use_psram=%u psram_free:%u->%u psram_used:%u->%u\r\n",
			 stage,
			 ptr,
			 probe_len,
			 (unsigned int)CONFIG_MBEDTLS_USE_PSRAM,
			 (unsigned int)free_before,
			 (unsigned int)free_after_alloc,
			 (unsigned int)used_before,
			 (unsigned int)used_after_alloc);

	mbedtls_free(ptr);
	CLI_LOGD("MEM %s after_free psram_free=%u psram_used=%u\r\n",
			 stage,
			 (unsigned int)rtos_get_psram_free_heap_size(),
			 (unsigned int)bk_psram_heap_get_used_count());
#else
	CLI_LOGD("MEM %s MBEDTLS_PLATFORM_MEMORY=0\r\n", stage);
#endif
}

static void mbedtls_tls_log_result(uint32_t *pass_cnt, uint32_t *fail_cnt, const char *test_name, int ret)
{
	if (ret == 0) {
		(*pass_cnt)++;
		CLI_LOGD("PASS test_name=%s ret=%d\r\n", test_name, ret);
	} else {
		(*fail_cnt)++;
		CLI_LOGE("FAIL test_name=%s ret=%d\r\n", test_name, ret);
	}
}

static void cli_mbedtls_sha_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_mbedtls_help();
		return;
	}

	uint32_t err_cnt = 0;
	int32_t ret = 0;

	//bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, MBEDTLS_TEST_FREQUENCY);

	if (os_strcmp(argv[1], "256") == 0) {
		for(int i = 0; i < sizeof(test_len)/sizeof(uint32_t); i++)
		{
			ret = te200_sha256_loop_test(test_len[i], MBEDTLS_SHA_TEST_CNT);
			err_if(ret != 0, ret);
		}
	}
	else if (os_strcmp(argv[1], "512") == 0) {

	}
	else {
		cli_mbedtls_help();
	}

	if (0 == err_cnt)
		CLI_LOGD("passed\r\n");
	else
		CLI_LOGE("failed\r\n");

	//bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, PM_CPU_FRQ_DEFAULT);
}

static void cli_mbedtls_aes_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_mbedtls_help();
		return;
	}

	uint32_t err_cnt = 0;
	int ret = 0;
	//bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, MBEDTLS_TEST_FREQUENCY);

	if (os_strcmp(argv[1], "ecb") == 0) {
		ret = te200_aes_ecb_test();
		err_if(ret != 0, ret);
	} 
	else if (os_strcmp(argv[1], "cbc") == 0) {
		ret = te200_aes_cbc_test();
		err_if(ret != 0, ret);

		for(int i = 0; i < sizeof(test_len)/sizeof(uint32_t); i++)
		{
			ret = te200_aes_cbc_large_data_test(test_len[i]);
			err_if(ret != 0, ret);
		}
	} 
	else if (os_strcmp(argv[1], "ctr") == 0) {
	 	ret = te200_aes_ctr_test();
		err_if(ret != 0, ret);
	}
	else if (os_strcmp(argv[1], "gcm") == 0) {
		for(int i = 0; i < sizeof(test_len)/sizeof(uint32_t); i++)
		{
			ret = te200_aes_gcm_large_data_test(test_len[i]);
			err_if(ret != 0, ret);
		}
	}
	else {
		cli_mbedtls_help();
	}

	if (0 == err_cnt)
		CLI_LOGD("passed\r\n");
	else
		CLI_LOGE("failed\r\n");

	//bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, PM_CPU_FRQ_DEFAULT);
}

static void cli_mbedtls_ecdsa_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_mbedtls_help();
		return;
	}

	int ret = 0;
	//bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, MBEDTLS_TEST_FREQUENCY);

	uint32_t loop_cnt = os_strtoul(argv[1], NULL, 10);
	ret = te200_ecdsa_self_test(1, loop_cnt);

	if (0 == ret)
		CLI_LOGD("passed\r\n");
	else
		CLI_LOGE("failed\r\n");

	//bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, PM_CPU_FRQ_DEFAULT);
}

static void cli_mbedtls_rsa_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = 0;
	//bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, MBEDTLS_TEST_FREQUENCY);

	ret = te200_rsa_self_test(1);

	if (0 == ret)
		CLI_LOGD("passed\r\n");
	else
		CLI_LOGE("failed\r\n");

	//bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, PM_CPU_FRQ_DEFAULT);
}

static void mbedtls_tls_run(uint32_t loop_cnt)
{
	uint32_t pass_cnt = 0;
	uint32_t fail_cnt = 0;
	int ret = 0;

	CLI_LOGD("mbedtls tls begin loops=%u\r\n", (unsigned int)loop_cnt);
	mbedtls_tls_log_mem_probe("tls_probe");

	ret = mbedtls_tls_server_certificate_test(loop_cnt);
	mbedtls_tls_log_result(&pass_cnt, &fail_cnt, "tls_server_certificate", ret);

	ret = mbedtls_tls_server_key_exchange_test(loop_cnt);
	mbedtls_tls_log_result(&pass_cnt, &fail_cnt, "tls_server_key_exchange", ret);

	ret = mbedtls_tls_client_key_exchange_test(loop_cnt);
	mbedtls_tls_log_result(&pass_cnt, &fail_cnt, "tls_client_key_exchange", ret);

	CLI_LOGD("tls summary: pass=%u fail=%u skip=0 final=%s\r\n",
			 (unsigned int)pass_cnt,
			 (unsigned int)fail_cnt,
			 (fail_cnt == 0U) ? "PASS" : "FAIL");
}

static void cli_mbedtls_tls_task(void *param)
{
	uint32_t loop_cnt = 1U;

	if (param != NULL) {
		loop_cnt = *(uint32_t *)param;
		os_free(param);
	}

	mbedtls_tls_run(loop_cnt);
	rtos_delete_thread(NULL);
}

static void cli_mbedtls_tls_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t *loop_cnt = NULL;

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	loop_cnt = os_zalloc(sizeof(uint32_t));
	if (loop_cnt == NULL) {
		CLI_LOGE("mbedtls_tls: no memory\r\n");
		return;
	}

	*loop_cnt = 1U;
	if ((argc >= 2) && (argv[1] != NULL)) {
		*loop_cnt = os_strtoul(argv[1], NULL, 10);
	}

	if (rtos_create_thread(NULL,
						   MBEDTLS_TEST_PRIORITY,
						   "mbedtls_tls",
						   cli_mbedtls_tls_task,
						   1024 * 5,
						   (beken_thread_arg_t)loop_cnt) != kNoErr) {
		CLI_LOGE("mbedtls_tls: create task failed\r\n");
		os_free(loop_cnt);
	}
}

static void cli_mbedtls_rand_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_mbedtls_help();
		return;
	}

	uint32_t err_cnt = 0;
	int ret = 0;

	if (os_strcmp(argv[1], "basic") == 0) {
		ret = te200_rand_basic_test();
		err_if(ret != 0, ret);
	} else if (os_strcmp(argv[1], "uniq") == 0) {
		ret = te200_rand_uniqueness_test();
		err_if(ret != 0, ret);
	} else if (os_strcmp(argv[1], "loop") == 0) {
		uint32_t loop_cnt = 1000;

		if (argc >= 3) {
			loop_cnt = os_strtoul(argv[2], NULL, 10);
		}
		ret = te200_rand_loop_test(loop_cnt);
		err_if(ret != 0, ret);
	} else {
		cli_mbedtls_help();
		return;
	}

	if (0 == err_cnt) {
		CLI_LOGD("passed\r\n");
	} else {
		CLI_LOGE("failed\r\n");
	}
}

static void cli_mbedtls_selftest(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = 0;

	//bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, MBEDTLS_TEST_FREQUENCY);

	extern int mbedtls_selftest_main(int argc, char *argv[]);
	ret = mbedtls_selftest_main(argc, argv);

	if (0 == ret)
		CLI_LOGD("passed\r\n");
	else
		CLI_LOGE("failed\r\n");

	//bk_pm_module_vote_cpu_freq(PM_DEV_ID_SECURE_WORLD, PM_CPU_FRQ_DEFAULT);
}

static uint32_t g_max_count;
static void cli_mbedtls_thread(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int ret = 0;

	if (argc < 2) {
		cli_mbedtls_help();
		return;
	}

	if (os_strcmp(argv[1], "create") == 0) {
		g_max_count = os_strtoul(argv[2], NULL, 10);
		BK_LOGD(NULL,"cli max counter %u\r\n", g_max_count);
		ret = rtos_create_thread(NULL,
							 MBEDTLS_TEST_PRIORITY,
							 "mbedtls_test",
							 (beken_thread_function_t)te200_muti_task_test,
							 1024*5,
							 (beken_thread_arg_t)(&g_max_count));
		if (ret != 0) {
			CLI_LOGE("Error: Failed to create mbedtls thread: %d\r\n", ret);
		}
	}else
	{
		cli_mbedtls_help();
	}
}

#define MBEDTLS_CMD_CNT (sizeof(s_mbedtls_commands) / sizeof(struct cli_command))
static const struct cli_command s_mbedtls_commands[] = {
	{"mbedtls_sha",      "mbedtls_sha {256|512}",         cli_mbedtls_sha_cmd},
	{"mbedtls_aes",      "mbedtls_aes {ecb|cbc|ctr|gcm}", cli_mbedtls_aes_cmd},
	{"mbedtls_ecdsa",    "mbedtls_ecdsa {10}",            cli_mbedtls_ecdsa_cmd},
	{"mbedtls_rsa",      "mbedtls_rsa",                   cli_mbedtls_rsa_cmd},
	{"mbedtls_rand",     "mbedtls_rand {basic|uniq|loop [cnt]}", cli_mbedtls_rand_cmd},
	{"mbedtls_tls",      "mbedtls_tls [cnt]",             cli_mbedtls_tls_cmd},
	{"mbedtls_selftest", "mbedtls_selftest",              cli_mbedtls_selftest},
	{"mbedtls_thread",   "mbedtls_thread {create}{count}",cli_mbedtls_thread},
};

int cli_mbedtls_init(void)
{
	return cli_register_commands(s_mbedtls_commands, MBEDTLS_CMD_CNT);
}
