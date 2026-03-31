#include <os/os.h>

#if (CONFIG_WANSON_ASR_GROUP_VERSION)
#include "fst_types.h"  /* Fst type only, no fst01_* arrays */
#endif

#if (CONFIG_WANSON_CN_LICENSE)
int  Wanson_ASR_Init(uint8_t userid[8])
{
	(void)userid;
	return 0;  /* 0 = OK, stub */
}
#else
int  Wanson_ASR_Init(void)
{
	return 0;  /* 0 = OK, stub */
}
#endif

#if(CONFIG_WANSON_ASR_GROUP_VERSION)
void Wanson_ASR_Set_Fst(Fst *fst)
{
	(void)fst;
}
#endif
void Wanson_ASR_Reset(void)
{
	return;
}

/*****************************************
* Input:
*      - buf      : Audio data (16k, 16bit, mono)
*      - buf_len  : Now must be 480 (30ms)
*
* Output:
*      - text     : The text of ASR
*      - score    : The confidence of ASR (Now not used)
*
* Return value    :  0 - No result
*                    1 - Has result
*                   -1 - Error
******************************************/
int  Wanson_ASR_Recog(short *buf, int buf_len, const char **text, float *score)
{
	(void)buf;
	(void)buf_len;
	(void)text;
	(void)score;
	return 0;  /* 0 = No result */
}

void Wanson_ASR_Release(void)
{
	return;
}


