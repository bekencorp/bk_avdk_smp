#include <stdio.h>

#include "bk_private/bk_init.h"
#include "components/bluetooth/bk_dm_bluetooth.h"
#include "components/system.h"
#include "media_service.h"

#include "le_audio_user_config.h"
#if CONFIG_LE_AUDIO_PROTOCOL_DEMO
#include "le_audio_protocol_demo.h"
#else
#include "bluetooth_app.h"
#endif

#define AUTO_ENABLE_LE_AUDIO_DEMO 1

int main(void)
{
	bk_init();
	media_service_init();

#if AUTO_ENABLE_LE_AUDIO_DEMO
#if CONFIG_LE_AUDIO_PROTOCOL_DEMO
	le_audio_demo_init();
#else
	bluetooth_app_init();
#endif

	extern int cli_le_audio_demo_init(void);
	cli_le_audio_demo_init();
#endif

	return 0;
}
