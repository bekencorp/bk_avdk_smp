#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/mem.h>
#include <os/os.h>
#include <stdint.h>

#define TAG "ap_fast_boot"

static volatile uint32_t s_ap_main_entry_count;
static beken_thread_t s_resume_test_thread;

static void ap_resume_test_task(void *arg)
{
	uint32_t sequence = 0;
	uint32_t stack_canary = 0x12345678U;
	uint32_t *heap_canary = os_malloc(sizeof(*heap_canary));

	(void)arg;
	if (heap_canary != NULL) {
		*heap_canary = 0xA55A5AA5U;
	}

	for (;;) {
		sequence++;
		BK_LOGI(TAG,
			"RESUME_PROOF seq=%u stack=0x%08x heap=%p heap_value=0x%08x "
			"main_entries=%u core=%u\r\n",
			sequence,
			stack_canary,
			heap_canary,
			(heap_canary != NULL) ? *heap_canary : 0U,
			s_ap_main_entry_count,
			rtos_get_core_id());
		rtos_delay_milliseconds(2000);
	}
}

int main(void)
{
	bk_err_t ret;

	bk_init();
	s_ap_main_entry_count++;
	BK_LOGI(TAG, "AP main entry count=%u\r\n", s_ap_main_entry_count);

	ret = rtos_create_thread(&s_resume_test_thread,
		BEKEN_DEFAULT_WORKER_PRIORITY,
		"ap_resume_test",
		ap_resume_test_task,
		2048,
		NULL);
	if (ret != BK_OK) {
		BK_LOGE(TAG, "create resume test task failed: %d\r\n", ret);
	}

	return 0;
}
