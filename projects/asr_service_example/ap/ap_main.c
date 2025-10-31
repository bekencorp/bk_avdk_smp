#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <media_service.h>
#include "cli_asr_service.h"

int main(void)
{
	bk_init();
    media_service_init();

    cli_asr_service_init();

	return 0;
}
