#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>

extern void rtos_set_user_app_entry(beken_thread_function_t entry);

static void user_app_main(void)
{
#if CONFIG_ADK
	extern int bk_audio_osi_funcs_init(void);
	bk_audio_osi_funcs_init();
#endif
#if CONFIG_VOICE_SERVICE && CONFIG_PLAYER_SERVICE
	extern int cli_voice_service_init(void);
	extern int cli_player_service_init(void);
	cli_voice_service_init();
	cli_player_service_init();
#endif
#if CONFIG_ASR_SERVICE && CONFIG_AUD_ASR_READ_SERVICE
	extern int cli_cp_audio_init(void);
	cli_cp_audio_init();
#endif
	os_printf("cp_audio_kws_example CP main running\n");
}

int main(void)
{
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
	bk_init();
	os_printf("cp_audio_kws_example CP init done\n");
	return 0;
}
