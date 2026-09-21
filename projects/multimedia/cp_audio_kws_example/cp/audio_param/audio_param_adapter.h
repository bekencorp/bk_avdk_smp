#ifndef __AUDIO_PARAM_ADAPTER_H__
#define __AUDIO_PARAM_ADAPTER_H__

#include <components/audio_param_ctrl.h>


void media_audio_param_bind_spk_handle(void *spk_handle);
void media_audio_param_unbind_spk_handle(void);
void media_audio_param_bind_voc_handle(void *voc_handle);
void media_audio_param_unbind_voc_handle(void);

#endif
