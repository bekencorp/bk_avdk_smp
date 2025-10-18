#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <media_service.h>
#include "cli_player_service.h"
#if (CONFIG_VOICE_SERVICE_TEST && CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE)
#include "cli_voice_service.h"
#endif

int main(void)
{
	bk_init();
    media_service_init();

    cli_player_service_init();
#if (CONFIG_VOICE_SERVICE_TEST && CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE)
    cli_voice_service_init();
#endif

	return 0;
}
