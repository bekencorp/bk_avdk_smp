// Copyright 2020-2024 Beken
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
#include "modules/ota.h"


#if CONFIG_HTTP_AB_PARTITION || CONFIG_SECURE_OTA_XIP
static void get_http_ab_version(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	exec_flag ret_partition = 0;

	ret_partition = bk_ota_get_current_partition();
	if(ret_partition == 0x0)
	{
    	BK_LOGD(NULL,"partition A\r\n");
    }
	else
	{
    	BK_LOGD(NULL,"partition B\r\n");
    }

}

extern int bk_ota_swap_execute_partition(void);
static void swap_ab_execute_partition(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	int32_t ret = 0;

	ret = bk_ota_swap_execute_partition();
	if(ret == BK_FAIL)
	{
		os_printf("swap fail\r\n");
	}
	else
	{
		os_printf("swap success\r\n");
		bk_reboot();
	}
}
#endif

#if (CONFIG_OTA_HTTP || CONFIG_OTA_HTTPS)
/*
 * HTTP/HTTPS OTA — vtable + inheritance, C-style:
 *
 *   Base "class":      bk_ota_job_t            (url / ops / busy)
 *   Method table:      bk_ota_job_ops_t        (name + download virtual)
 *                      One static const instance per concrete "class":
 *                      HTTP_LEGACY_OPS / HTTP_NEW_OPS / HTTPS_OPS.
 *                      The whole table lives in .rodata, immutable.
 *   Derived "class":   bk_ota_job_new_t extends bk_ota_job_t,
 *                      adds dest_id (only consumed by http_new strategy).
 *
 *   The base is placed as the FIRST field of the derived struct, so a
 *   bk_ota_job_t* and a bk_ota_job_new_t* share the same address.
 *   This lets the shared worker thread / start-method treat every job
 *   uniformly via the base pointer, while a strategy that needs the
 *   extra field (http_ota_new_download) downcasts back to the derived
 *   type — exactly the C analogue of OO inheritance + polymorphism.
 *
 *   bk_ota_job_init()         : shared "constructor" — sets url + ops.
 *   bk_ota_job_start()        : member method, concurrency check + thread.
 *   bk_ota_job_thread_entry() : common worker entry, dispatches via
 *                               job->ops->download(job).
 *
 * Note: each strategy / handler that touches an underlying API
 * (bk_http_ota_download / bk_ota_start_download / bk_https_ota_download)
 * remains guarded by CONFIG_OTA_HTTP / CONFIG_OTA_HTTPS — those symbols
 * are only linked in when their corresponding config is enabled (see
 * ap/components/ota/CMakeLists.txt).
 */

/* Forward typedef — required: bk_ota_download_fn (below) references
 * bk_ota_job_t * before struct bk_ota_job_s is fully defined. */

typedef struct bk_ota_job_s bk_ota_job_t;

/* Virtual function pointer type — base of polymorphic dispatch. */

typedef int (*bk_ota_download_fn)(bk_ota_job_t *job);

/* ---- Method table ("vtable") — one static const instance per concrete
 *      class. Lives in .rodata, never mutated at runtime. ---- */
typedef struct {
	const char            *name;     /* thread name / log tag */
	bk_ota_download_fn     download; /* polymorphic dispatch */
} bk_ota_job_ops_t;

/* ---- Base "class" ---- */
struct bk_ota_job_s {
	const char                   *url;
	const bk_ota_job_ops_t       *ops;   /* "class pointer", points into .rodata */
	volatile int                  busy;  /* 0 = idle, 1 = downloading */
};

/* ---- Derived "class": extends base with dest_id ---- */
typedef struct {
	bk_ota_job_t           base;     /* MUST be first field for upcast safety */
	ota_wr_destination_t   dest_id;
} bk_ota_job_new_t;

/* Single shared instance. Declared as the derived type so it also has room
 * for dest_id; strategies that don't need it simply ignore that field. */
static bk_ota_job_new_t s_ota_job;
#define OTA_JOB_BASE(p)    (&(p)->base)

/* ---- Shared "constructor" ---- */
static inline void bk_ota_job_init(bk_ota_job_t *job,
								   const char *url,
								   const bk_ota_job_ops_t *ops)
{
	job->url = url;
	job->ops = ops;
	/* busy is managed by thread entry/exit; do not reset here. */
}

/* ---- Shared worker thread entry ---- */
static void bk_ota_job_thread_entry(beken_thread_arg_t arg)
{
	bk_ota_job_t *job = (bk_ota_job_t *)arg;

	job->busy = 1;
	int ret = job->ops->download(job);
	if (0 != ret)
		BK_LOGE(NULL, "%s download failed, ret:%d\r\n", job->ops->name, ret);
	job->busy = 0;

	rtos_delete_thread(NULL);
}

/* ---- Shared start method ---- */
static int bk_ota_job_start(bk_ota_job_t *job)
{
	int ret;

	if (job->busy) {
		BK_LOGD(NULL, "already do ota and do wait it finished\r\n");
		return kGeneralErr;
	}

#if CONFIG_SOC_SMP
	ret = rtos_core0_create_thread(NULL,
                                  BEKEN_APPLICATION_PRIORITY,
                                  job->ops->name,
                                  (beken_thread_function_t)bk_ota_job_thread_entry,
                                  5120,
                                  (beken_thread_arg_t)job);
#else
	ret = rtos_create_thread(NULL,
                            BEKEN_APPLICATION_PRIORITY,
                            job->ops->name,
                            (beken_thread_function_t)bk_ota_job_thread_entry,
                            5120,
                            (beken_thread_arg_t)job);
#endif

	if (kNoErr != ret)
		BK_LOGE(NULL, "%s start failed, ret:%d\r\n", job->ops->name, ret);

	return ret;
}

/* ---- Concrete strategies + their method tables (each guarded by the API
 *      that backs it). ---- */

#if CONFIG_OTA_HTTP
/* Strategy 1: legacy HTTP — uses only base fields. */
static int http_ota_legacy_download(bk_ota_job_t *job)
{
	return bk_http_ota_download(job->url);
}

/* Strategy 2: new HTTP — needs dest_id from the derived class.
 * Safe downcast: base lives at offset 0 of the derived type. */
static int http_ota_new_download(bk_ota_job_t *job)
{
	bk_ota_job_new_t *self = (bk_ota_job_new_t *)job;
	return bk_ota_start_download(self->base.url, self->dest_id);
}

static const bk_ota_job_ops_t HTTP_LEGACY_OPS = {
	.name     = "http_ota",
	.download = http_ota_legacy_download,
};

static const bk_ota_job_ops_t HTTP_NEW_OPS = {
	.name     = "http_new_ota",
	.download = http_ota_new_download,
};
#endif  /* CONFIG_OTA_HTTP */

#if CONFIG_OTA_HTTPS
int bk_https_ota_download(const char *url);  /* forward decl */

/* Strategy 3: HTTPS — uses only base fields. */
static int http_ota_https_download(bk_ota_job_t *job)
{
	return bk_https_ota_download(job->url);
}

static const bk_ota_job_ops_t HTTPS_OPS = {
	.name     = "https_ota",
	.download = http_ota_https_download,
};
#endif  /* CONFIG_OTA_HTTPS */

/* ---- CLI handlers — just call the shared constructor + start ---- */

#if CONFIG_OTA_HTTP
static void http_ota_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		BK_LOGE(NULL, "Usage: http_ota <url>\r\n");
		return;
	}

	bk_ota_job_init(OTA_JOB_BASE(&s_ota_job), argv[1], &HTTP_LEGACY_OPS);

	(void)bk_ota_job_start(OTA_JOB_BASE(&s_ota_job));
}

static void http_new_ota_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 3) {
		BK_LOGE(NULL, "Usage: http_new_ota <url> <dest_id>\r\n");
		return;
	}

	bk_ota_job_init(OTA_JOB_BASE(&s_ota_job), argv[1], &HTTP_NEW_OPS);
	s_ota_job.dest_id = (ota_wr_destination_t)os_strtoul(argv[2], NULL, 10);
	BK_LOGD(NULL, "dest_id :%d\r\n", s_ota_job.dest_id);

	(void)bk_ota_job_start(OTA_JOB_BASE(&s_ota_job));
}
#endif  /* CONFIG_OTA_HTTP */

#if CONFIG_OTA_HTTPS
static void https_ota_Command(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc != 2) {
		BK_LOGE(NULL, "%s, Usage: https_ota <url>\r\n", __func__);
		return;
	}

	bk_ota_job_init(OTA_JOB_BASE(&s_ota_job), argv[1], &HTTPS_OPS);

	(void)bk_ota_job_start(OTA_JOB_BASE(&s_ota_job));
}
#endif  /* CONFIG_OTA_HTTPS */
#endif  /* CONFIG_OTA_HTTP || CONFIG_OTA_HTTPS */

#define OTA_CMD_CNT (sizeof(s_ota_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_ota_commands[] = {

#if CONFIG_OTA_HTTP
	{"http_ota", "http_ota url", http_ota_Command},
	{"http_new_ota", "http_ota url [dest_id]", http_new_ota_Command},
#endif

#if CONFIG_OTA_HTTPS
	{"https_ota", "ip [sta|ap][{ip}{mask}{gate}{dns}]", https_ota_Command},

#endif

#if CONFIG_HTTP_AB_PARTITION ||CONFIG_SECURE_OTA_XIP	
	{"ab_version", NULL, get_http_ab_version},
#if CONFIG_HTTP_AB_PARTITION
	{"swap_ab_partition", NULL, swap_ab_execute_partition},
#endif
#endif
};

#if (CONFIG_TFM_FWU)
extern int32_t ns_interface_lock_init(void);
#endif

int bk_ota_register_cli_test_feature(void)
{
#if (CONFIG_TFM_FWU)
	ns_interface_lock_init();
#endif
	return cli_register_module_test_feature(s_ota_commands, OTA_CMD_CNT);
}
