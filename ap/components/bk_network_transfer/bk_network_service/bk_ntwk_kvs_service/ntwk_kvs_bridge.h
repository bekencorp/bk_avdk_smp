#pragma once

#if CONFIG_KVS_NTWK_BRIDGE

#include <com/amazonaws/kinesis/video/webrtcclient/Include.h>

/* streaming_session: opaque pointer for detach matching only (e.g. sample session).
 * video/audio transceivers: from the same session; avoids coupling this component to Samples.h. */
void ntwk_kvs_bridge_attach_session(void *streaming_session, PRtcRtpTransceiver video_transceiver,
				    PRtcRtpTransceiver audio_transceiver);
void ntwk_kvs_bridge_detach_session(void *streaming_session);

#endif
