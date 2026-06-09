#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <media_service.h>
#include "cli_audio_player.h"

int main(void)
{
	bk_init();
    media_service_init();

    cli_audio_player_init();

	return 0;
}
