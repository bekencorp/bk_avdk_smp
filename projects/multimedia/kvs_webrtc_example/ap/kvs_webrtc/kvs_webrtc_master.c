/**
 * KVS master for doorbell_kvs: signaling + WebRTC like kvsWebRTCClientMaster, but media
 * comes from ntwk_trans_video_send / ntwk_trans_audio_send (no disk sample sender threads).
 */
#include "kvs_common.h"
#include "ntwk_kvs_service.h"


extern PSampleConfiguration gSampleConfiguration;

INT32 kvs_webrtc_master_main(INT32 argc, CHAR *argv[])
{
	STATUS retStatus = STATUS_SUCCESS;
	PSampleConfiguration pSampleConfiguration = NULL;
	PCHAR pChannelName;
	SignalingClientMetrics signalingClientMetrics;
	signalingClientMetrics.version = SIGNALING_CLIENT_METRICS_CURRENT_VERSION;
	RTC_CODEC videoCodec = RTC_CODEC_H264_PROFILE_42E01F_LEVEL_ASYMMETRY_ALLOWED_PACKETIZATION_MODE;
	RTC_CODEC audioCodec = RTC_CODEC_ALAW;

	SET_INSTRUMENTED_ALLOCATORS();
	UINT32 logLevel = setLogLevel();

#ifdef IOT_CORE_ENABLE_CREDENTIALS
	CHK_ERR((pChannelName = argc > 1 ? argv[1] : GETENV(IOT_CORE_THING_NAME)) != NULL, STATUS_INVALID_OPERATION,
		"AWS_IOT_CORE_THING_NAME must be set");
#else
	pChannelName = argc > 1 ? argv[1] : SAMPLE_CHANNEL_NAME;
#endif

	CHK_STATUS(createSampleConfiguration(pChannelName, SIGNALING_CHANNEL_ROLE_TYPE_MASTER, TRUE, TRUE, logLevel, &pSampleConfiguration));

	/*
	 * Rolling buffer capacity (packets) ≈ duration_sec * bitrate_bps / (8 * DEFAULT_MTU_SIZE_BYTES)
	 *   with DEFAULT_MTU_SIZE_BYTES = 1200 → divisor 9600 (see SessionDescription.c + Rtp.h).
	 * SDK default is 3 s × 10 Mbps → ~3276 slots per transceiver (too heavy on embedded heap).
	 * Here: video ~262 slots (1.2 s @ 2 Mbps), audio ~13 slots (1 s @ 128 kbps).
	 * Note: bitrate must be >= MIN_EXPECTED_BIT_RATE (~102.4 kbps); do not use 64000 for G.711
	 * or configureTransceiverRollingBuffer will clamp back to the 10 Mbps default.
	 */
	pSampleConfiguration->videoRollingBufferDurationSec = 1.2;
	pSampleConfiguration->videoRollingBufferBitratebps = (DOUBLE)(2 * 1024 * 1024);
	pSampleConfiguration->audioRollingBufferDurationSec = 1.0;
	pSampleConfiguration->audioRollingBufferBitratebps = (DOUBLE)(128 * 1024);

	pSampleConfiguration->videoCodec = videoCodec;
	pSampleConfiguration->audioCodec = audioCodec;
	pSampleConfiguration->videoSource = NULL;
	pSampleConfiguration->audioSource = NULL;
	pSampleConfiguration->receiveAudioVideoSource = sampleReceiveAudioVideoFrame;
#ifdef ENABLE_DATA_CHANNEL
	pSampleConfiguration->onDataChannel = (RtcOnDataChannel)ntwk_kvs_on_data_channel;
#endif
	pSampleConfiguration->mediaType = SAMPLE_STREAMING_AUDIO_VIDEO;

	CHK_STATUS(initKvsWebRtc());

	PROFILE_CALL_WITH_START_END_T_OBJ(
		retStatus = initSignaling(pSampleConfiguration, SAMPLE_MASTER_CLIENT_ID), pSampleConfiguration->signalingClientMetrics.signalingStartTime,
		pSampleConfiguration->signalingClientMetrics.signalingEndTime, pSampleConfiguration->signalingClientMetrics.signalingCallTime,
		"Initialize signaling client and connect to the signaling channel");

	CHK_STATUS(sessionCleanupWait(pSampleConfiguration));

CleanUp:

	if (retStatus != STATUS_SUCCESS) {
		DLOGE("[KVS Doorbell] Terminated with status code 0x%08x", retStatus);
	}

	if (pSampleConfiguration != NULL) {
		ATOMIC_STORE_BOOL(&pSampleConfiguration->appTerminateFlag, TRUE);

		if (pSampleConfiguration->mediaSenderTid != INVALID_TID_VALUE) {
			THREAD_JOIN(pSampleConfiguration->mediaSenderTid, NULL);
		}

		retStatus = signalingClientGetMetrics(pSampleConfiguration->signalingClientHandle, &signalingClientMetrics);
		if (retStatus == STATUS_SUCCESS) {
			logSignalingClientStats(&signalingClientMetrics);
		}
		retStatus = freeSignalingClient(&pSampleConfiguration->signalingClientHandle);
		retStatus = freeSampleConfiguration(&pSampleConfiguration);
	}

	gSampleConfiguration = NULL;
	CHK_LOG_ERR(retStatus);
	RESET_INSTRUMENTED_ALLOCATORS();

	return STATUS_FAILED(retStatus) ? 1 : 0;
}

PVOID sampleReceiveAudioVideoFrame(PVOID args)
{
	STATUS retStatus = STATUS_SUCCESS;
	PSampleStreamingSession pSampleStreamingSession = (PSampleStreamingSession)args;

	CHK_ERR(pSampleStreamingSession != NULL, STATUS_NULL_ARG, "[KVS] streaming session is NULL");
	CHK_STATUS(transceiverOnFrame(pSampleStreamingSession->pVideoRtcRtpTransceiver, (UINT64)(uintptr_t)pSampleStreamingSession,
				      sampleVideoFrameHandler));
	CHK_STATUS(transceiverOnFrame(pSampleStreamingSession->pAudioRtcRtpTransceiver, (UINT64)(uintptr_t)pSampleStreamingSession,
				      sampleAudioFrameHandler));

CleanUp:

	return (PVOID)(uintptr_t)retStatus;
}
