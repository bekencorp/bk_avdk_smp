#include <os/os.h>

#include <common/bk_include.h>
#include <components/bk_audio_asr_service.h>
#include <components/bk_asr_service_types.h>
#include <components/bk_asr_service.h>

#include "asr.h"


#if(CONFIG_WANSON_ASR_GROUP_VERSION)
Fst fst_1;
Fst fst_2;
static unsigned char asr_curr_group_id; // 当前使用的分组ID

static uint8_t __maybe_unused wanson_fst_group_select = 0;
#endif


#if(CONFIG_WANSON_ASR_GROUP_VERSION)
/**
 * @brief 分组设置 
 * 
 * 当前设备没有播放音乐时，切换成分组1
 * 当设备需要播放音乐前，将其切换成分组2
 * 
 * @param group_id 分组ID
 */
void wanson_fst_group_change(unsigned char group_id)
{
    /* 如果需要设置的分组和当前分组ID不一致，则进行切换 */
    if(asr_curr_group_id != group_id) {
        asr_curr_group_id = group_id;

        if (group_id == 1) {
            Wanson_ASR_Set_Fst(&fst_1);
        } else if (group_id == 2) {
            Wanson_ASR_Set_Fst(&fst_2);
        }
        os_printf("fst_group_change_to: %d\n", group_id);
    }
}
#endif


int bk_wanson_armino_asr_init(void)
{
	int res = Wanson_ASR_Init();
	if (res < 0)
	{
		os_printf("Wanson_ASR_Init Failed!\n");
		return res;
	}
#if (CONFIG_WANSON_ASR_GROUP_VERSION)
	/* 指令分组初始化 */
	fst_1.states = fst01_states;
	fst_1.num_states = fst01_num_states;
	fst_1.finals = fst01_finals;
	fst_1.num_finals = fst01_num_finals;
	fst_1.words = fst01_words;

	fst_2.states = fst02_states;
	fst_2.num_states = fst02_num_states;
	fst_2.finals = fst02_finals;
	fst_2.num_finals = fst02_num_finals;
	fst_2.words = fst02_words;

	/* 设置默认分组 */
	wanson_fst_group_change(2); // 默认设置分组一
	os_printf("Wanson_ASR_Init GRP OK!\n");
#else
	Wanson_ASR_Reset();
#endif
	return res;
}


void bk_wanson_armino_asr_deinit(void)
{
	Wanson_ASR_Release();
}


int bk_wanson_armino_asr_recog(void *read_buf, uint32_t read_size, void *p1, void *p2)
{
	return Wanson_ASR_Recog((short*)read_buf, read_size>>1, p1, p2);
}


