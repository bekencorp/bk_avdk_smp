#include "audio_mp52_ipc_cp.h"

#if CONFIG_AEC_RUN_ON_M52
void bk_module_init(void)
{
    (void)aec_m52_ipc_cp_init();
}
#endif
