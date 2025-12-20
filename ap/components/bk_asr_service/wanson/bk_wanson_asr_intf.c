#include <os/os.h>

#include <common/bk_include.h>
#include <components/bk_audio_asr_service.h>
#include <components/bk_asr_service_types.h>
#include <components/bk_asr_service.h>

#include "asr.h"

#if(CONFIG_WANSON_ASR_GROUP_VERSION)
/* Group-related global variables */
Fst fst_1;
Fst fst_2;
static unsigned char asr_curr_group_id; // Currently used group ID
static uint8_t __maybe_unused wanson_fst_group_select = 0;

/**
 * @brief Group setting 
 * 
 * When the device is not playing music, switch to group 1
 * When the device needs to play music, switch to group 2
 * 
 * @param group_id Group ID
 */
void wanson_fst_group_change(unsigned char group_id)
{
    /* Switch if the target group ID is different from current group ID */
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

/**
 * @brief ASR common initialization function
 * 
 * @param with_group Whether to enable group function
 * @return int Initialization result
 */
int bk_wanson_asr_common_init(void)
{
	int res = Wanson_ASR_Init();
	if (res < 0)
	{
		os_printf("Wanson_ASR_Init Failed!\n");
		return res;
	}

#if (CONFIG_WANSON_ASR_GROUP_VERSION)
	/* Command group initialization */
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

	/* Set default group */
	wanson_fst_group_change(2);
	os_printf("Wanson_ASR_Init GRP OK!\n");
	return res;
#else
	Wanson_ASR_Reset();
#endif
	return res;
}

/**
 * @brief ASR deinitialization
 * 
 * Release ASR resources
 */
void bk_wanson_asr_common_deinit(void)
{
	Wanson_ASR_Release();
}

/**
 * @brief ASR recognition function
 * 
 * @param read_buf Audio data buffer
 * @param read_size Audio data size
 * @param p1 User parameter 1
 * @param p2 User parameter 2
 * @return int Recognition result
 */
int bk_wanson_asr_recog(void *read_buf, uint32_t read_size, void *p1, void *p2)
{
	return Wanson_ASR_Recog((short*)read_buf, read_size>>1, (const char**)p1, p2);
}

