#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>

int main(void)
{
	bk_init();
	os_printf("cp_audio_kws_example AP main running\n");
	return 0;
}
