#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/mem.h>
#include <os/os.h>
#include <stdint.h>
#include <media_service.h>

#include "cli_asr_service.h"
#include "cli_player_service.h"
#include "cli_voice_service.h"
#if CONFIG_AUD_PM_FAST_COLD || CONFIG_AUD_PM_FAST_HOT
#include "cli_aud_fb_test.h"
#endif

#define TAG "aud_fb_ap"

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

	/*
	 * Print once at INFO so retention canaries are visible after cold boot.
	 * Keep the 2s proof at DEBUG: INFO on the shared UART collides with
	 * CP's power-on callback window and inflates AP_TIME startup_success.
	 */
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

	for (;;) {
		rtos_delay_milliseconds(2000);
		sequence++;
		BK_LOGD(TAG,
			"RESUME_PROOF seq=%u stack=0x%08x heap=%p heap_value=0x%08x "
			"main_entries=%u core=%u\r\n",
			sequence,
			stack_canary,
			heap_canary,
			(heap_canary != NULL) ? *heap_canary : 0U,
			s_ap_main_entry_count,
			rtos_get_core_id());
	}
}

int main(void)
{
	bk_err_t ret;

	bk_init();
	s_ap_main_entry_count++;
	BK_LOGI(TAG, "AP main entry count=%u\r\n", s_ap_main_entry_count);

	media_service_init();

#if CONFIG_ASR_SERVICE
	cli_asr_service_init();
#endif
#if CONFIG_PLAYER_SERVICE
	cli_player_service_init();
#endif
#if CONFIG_VOICE_SERVICE_TEST
	cli_voice_service_init();
#endif
#if CONFIG_AUD_PM_FAST_COLD || CONFIG_AUD_PM_FAST_HOT
	cli_aud_fb_test_init();
#endif

	ret = rtos_create_thread(&s_resume_test_thread,
		BEKEN_DEFAULT_WORKER_PRIORITY,
		"ap_resume_test",
		ap_resume_test_task,
		2048,
		NULL);
	if (ret != BK_OK) {
		BK_LOGE(TAG, "create resume test task failed: %d\r\n", ret);
	}

#if CONFIG_AUD_PM_FAST_COLD || CONFIG_AUD_PM_FAST_HOT
	BK_LOGI(TAG, "CLI ready: aud_fb service voice|asr|player init / stop / deinit\r\n");
#endif
	return 0;
}
