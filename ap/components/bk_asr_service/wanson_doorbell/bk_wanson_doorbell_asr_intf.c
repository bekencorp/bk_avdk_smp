#include <os/os.h>

#include <common/bk_include.h>
#include <components/bk_audio_asr_service.h>
#include <components/bk_asr_service_types.h>
#include <components/bk_asr_service.h>

#include "asr.h"



int bk_wanson_doorbell_asr_init(void)
{
	int res = Wanson_ASR_Init();
	if (res < 0)
	{
		os_printf("Wanson_ASR_Init Failed!\n");
		return res;
	}

	Wanson_ASR_Reset();
	return res;
}


void bk_wanson_doorbell_asr_deinit(void)
{
	Wanson_ASR_Release();
}


int bk_wanson_doorbell_asr_recog(void *read_buf, uint32_t read_size, void *p1, void *p2)
{
	return Wanson_ASR_Recog((short*)read_buf, read_size>>1, p1, p2);
}


