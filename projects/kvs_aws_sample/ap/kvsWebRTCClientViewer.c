/**
 * KVS WebRTC Viewer: receive H264/H265/Opus from Master.
 * Frames are delivered to sampleVideoFrameHandler/sampleAudioFrameHandler in Common.c.
 */
#include "Samples.h"
#if !defined(_WIN32) && !defined(BEKEN_PLATFORM)
#include <signal.h>
#endif

extern PSampleConfiguration gSampleConfiguration;

#ifdef ENABLE_DATA_CHANNEL

// onMessage callback for a message received by the viewer on a data channel
VOID dataChannelOnMessageCallback(UINT64 customData, PRtcDataChannel pDataChannel, BOOL isBinary, PBYTE pMessage, UINT32 pMessageLen)
{
    UNUSED_PARAM(customData);
    UNUSED_PARAM(pDataChannel);
    if (isBinary) {
        DLOGI("DataChannel Binary Message");
    } else {
        DLOGI("DataChannel String Message: %.*s", pMessageLen, pMessage);
    }
}

// onOpen callback for the onOpen event of a viewer created data channel
VOID dataChannelOnOpenCallback(UINT64 customData, PRtcDataChannel pDataChannel)
{
    STATUS retStatus = STATUS_SUCCESS;
    DLOGI("New DataChannel has been opened %s ", pDataChannel->name);
    dataChannelOnMessage(pDataChannel, customData, dataChannelOnMessageCallback);
    ATOMIC_INCREMENT((PSIZE_T) customData);
    retStatus = dataChannelSend(pDataChannel, FALSE, (PBYTE) VIEWER_DATA_CHANNEL_MESSAGE, STRLEN(VIEWER_DATA_CHANNEL_MESSAGE));
    if (retStatus != STATUS_SUCCESS) {
        DLOGI("[KVS Viewer] dataChannelSend(): operation returned status code: 0x%08x ", retStatus);
    }
}
#endif

INT32 kvs_aws_viewer_main(INT32 argc, CHAR* argv[])
{
    STATUS retStatus = STATUS_SUCCESS;
    #if 0
    RtcSessionDescriptionInit offerSessionDescriptionInit = {0};
    #else
    PRtcSessionDescriptionInit pOfferSessionDescriptionInit = NULL;
    #endif
    UINT32 buffLen = 0;
    #if 0
    SignalingMessage message = {0};
    #else
    PSignalingMessage pSignalingMessage = NULL;
    #endif
    PSampleConfiguration pSampleConfiguration = NULL;
    PSampleStreamingSession pSampleStreamingSession = NULL;
    RTC_CODEC audioCodec = RTC_CODEC_OPUS;
    RTC_CODEC videoCodec = RTC_CODEC_H264_PROFILE_42E01F_LEVEL_ASYMMETRY_ALLOWED_PACKETIZATION_MODE;
    BOOL locked = FALSE;
    PCHAR pChannelName;
    CHAR clientId[256];

    SET_INSTRUMENTED_ALLOCATORS();
    UINT32 logLevel = setLogLevel();

#if !defined(_WIN32) && !defined(BEKEN_PLATFORM)
    signal(SIGINT, sigintHandler);
#endif

    #if 1
    pOfferSessionDescriptionInit = (PRtcSessionDescriptionInit) MEMCALLOC(1, SIZEOF(RtcSessionDescriptionInit));
    CHK(pOfferSessionDescriptionInit != NULL, STATUS_NOT_ENOUGH_MEMORY);
    pSignalingMessage = (PSignalingMessage) MEMCALLOC(1, SIZEOF(SignalingMessage));
    CHK(pSignalingMessage != NULL, STATUS_NOT_ENOUGH_MEMORY);
    #endif


#ifdef IOT_CORE_ENABLE_CREDENTIALS
    CHK_ERR((pChannelName = argc > 1 ? argv[1] : GETENV(IOT_CORE_THING_NAME)) != NULL, STATUS_INVALID_OPERATION,
            "AWS_IOT_CORE_THING_NAME must be set");
#else
    pChannelName = argc > 1 ? argv[1] : SAMPLE_CHANNEL_NAME;
#endif

    if (argc > 2) {
        if (!STRCMP(argv[2], AUDIO_CODEC_NAME_OPUS)) {
            audioCodec = RTC_CODEC_OPUS;
        } else if (!STRCMP(argv[2], AUDIO_CODEC_NAME_ALAW)) {
            audioCodec = RTC_CODEC_ALAW;
        } else if (!STRCMP(argv[2], AUDIO_CODEC_NAME_MULAW)) {
            audioCodec = RTC_CODEC_MULAW;
        } else {
            DLOGI("[KVS Viewer] Defaulting to Opus audio codec");
        }
    }

    if (argc > 3) {
        if (!STRCMP(argv[3], VIDEO_CODEC_NAME_H265)) {
            videoCodec = RTC_CODEC_H265;
        } else if (!STRCMP(argv[3], VIDEO_CODEC_NAME_VP8)) {
            videoCodec = RTC_CODEC_VP8;
        } else {
            DLOGI("[KVS Viewer] Defaulting to H264 video codec");
        }
    }

    CHK_STATUS(createSampleConfiguration(pChannelName, SIGNALING_CHANNEL_ROLE_TYPE_VIEWER, TRUE, TRUE, logLevel, &pSampleConfiguration));
    pSampleConfiguration->mediaType = SAMPLE_STREAMING_AUDIO_VIDEO;
    pSampleConfiguration->audioCodec = audioCodec;
    pSampleConfiguration->videoCodec = videoCodec;

    CHK_STATUS(initKvsWebRtc());
    DLOGI("[KVS Viewer] KVS WebRTC initialization completed successfully");

#ifdef ENABLE_DATA_CHANNEL
    pSampleConfiguration->onDataChannel = onDataChannel;
#endif

    SPRINTF(clientId, "%s_%u", SAMPLE_VIEWER_CLIENT_ID, RAND() % MAX_UINT32);
    CHK_STATUS(initSignaling(pSampleConfiguration, clientId));
    DLOGI("[KVS Viewer] Signaling client connection established");

    MUTEX_LOCK(pSampleConfiguration->sampleConfigurationObjLock);
    locked = TRUE;
    CHK_STATUS(createSampleStreamingSession(pSampleConfiguration, NULL, FALSE, &pSampleStreamingSession));
    DLOGI("[KVS Viewer] Creating streaming session...completed");
    pSampleConfiguration->sampleStreamingSessionList[pSampleConfiguration->streamingSessionCount++] = pSampleStreamingSession;

    MUTEX_UNLOCK(pSampleConfiguration->sampleConfigurationObjLock);
    locked = FALSE;

    #if 0
    MEMSET(&offerSessionDescriptionInit, 0x00, SIZEOF(RtcSessionDescriptionInit));
    #else
    MEMSET(pOfferSessionDescriptionInit, 0x00, SIZEOF(RtcSessionDescriptionInit));
    #endif

    #if 0
    offerSessionDescriptionInit.useTrickleIce = pSampleStreamingSession->remoteCanTrickleIce;
    CHK_STATUS(setLocalDescription(pSampleStreamingSession->pPeerConnection, &offerSessionDescriptionInit));
    #else
    pOfferSessionDescriptionInit->useTrickleIce = pSampleStreamingSession->remoteCanTrickleIce;
    CHK_STATUS(setLocalDescription(pSampleStreamingSession->pPeerConnection, pOfferSessionDescriptionInit));
    #endif

    DLOGI("[KVS Viewer] Completed setting local description");

    CHK_STATUS(transceiverOnFrame(pSampleStreamingSession->pAudioRtcRtpTransceiver, (UINT64)(uintptr_t) pSampleStreamingSession, sampleAudioFrameHandler));
    CHK_STATUS(transceiverOnFrame(pSampleStreamingSession->pVideoRtcRtpTransceiver, (UINT64)(uintptr_t) pSampleStreamingSession, sampleVideoFrameHandler));

    if (!pSampleConfiguration->trickleIce) {
        DLOGI("[KVS Viewer] Non trickle ice. Wait for Candidate collection to complete");
        MUTEX_LOCK(pSampleConfiguration->sampleConfigurationObjLock);
        locked = TRUE;

        while (!ATOMIC_LOAD_BOOL(&pSampleStreamingSession->candidateGatheringDone)) {
            CHK_WARN(!ATOMIC_LOAD_BOOL(&pSampleStreamingSession->terminateFlag), STATUS_OPERATION_TIMED_OUT,
                     "application terminated and candidate gathering still not done");
            CVAR_WAIT(pSampleConfiguration->cvar, pSampleConfiguration->sampleConfigurationObjLock, 5 * HUNDREDS_OF_NANOS_IN_A_SECOND);
        }

        MUTEX_UNLOCK(pSampleConfiguration->sampleConfigurationObjLock);
        locked = FALSE;

        DLOGI("[KVS Viewer] Candidate collection completed");
    }

    #if 0
    CHK_STATUS(createOffer(pSampleStreamingSession->pPeerConnection, &offerSessionDescriptionInit));
    #else
    CHK_STATUS(createOffer(pSampleStreamingSession->pPeerConnection, pOfferSessionDescriptionInit));
    #endif
    DLOGI("[KVS Viewer] Offer creation successful");

    DLOGI("[KVS Viewer] Generating JSON of session description....");
    #if 0
    CHK_STATUS(serializeSessionDescriptionInit(&offerSessionDescriptionInit, NULL, &buffLen));
    #else
    CHK_STATUS(serializeSessionDescriptionInit(pOfferSessionDescriptionInit, NULL, &buffLen));
    #endif

    #if 0
    if (buffLen >= SIZEOF(message.payload))
    #else
    if (buffLen >= SIZEOF(pSignalingMessage->payload))
    #endif
    {
        DLOGE("[KVS Viewer] serializeSessionDescriptionInit(): operation returned status code: 0x%08x ", STATUS_INVALID_OPERATION);
        retStatus = STATUS_INVALID_OPERATION;
        goto CleanUp;
    }

    #if 0
    CHK_STATUS(serializeSessionDescriptionInit(&offerSessionDescriptionInit, message.payload, &buffLen));
    #else
    CHK_STATUS(serializeSessionDescriptionInit(pOfferSessionDescriptionInit, pSignalingMessage->payload, &buffLen));
    #endif

    #if 0
    message.version = SIGNALING_MESSAGE_CURRENT_VERSION;
    message.messageType = SIGNALING_MESSAGE_TYPE_OFFER;
    STRCPY(message.peerClientId, SAMPLE_MASTER_CLIENT_ID);
    message.payloadLen = (buffLen / SIZEOF(CHAR)) - 1;
    message.correlationId[0] = '\0';
    CHK_STATUS(signalingClientSendMessageSync(pSampleConfiguration->signalingClientHandle, &message));
    #else
    pSignalingMessage->version = SIGNALING_MESSAGE_CURRENT_VERSION;
    pSignalingMessage->messageType = SIGNALING_MESSAGE_TYPE_OFFER;
    STRCPY(pSignalingMessage->peerClientId, SAMPLE_MASTER_CLIENT_ID);
    pSignalingMessage->payloadLen = (buffLen / SIZEOF(CHAR)) - 1;
    #if 0
    pSignalingMessage->correlationId[0] = '\0';
    CHK_STATUS(signalingClientSendMessageSync(pSampleConfiguration->signalingClientHandle, pSignalingMessage));
    #else
    /* Unique id: empty correlationId collides with ICE messages in signalingStoreOngoingMessage(); use sendSignalingMessage for the same lock as ICE. */
    SNPRINTF(pSignalingMessage->correlationId, MAX_CORRELATION_ID_LEN, "offer_%" PRIu64 "_%" PRIu64, GETTIME(),
             ATOMIC_INCREMENT(&pSampleStreamingSession->correlationIdPostFix));
    CHK_STATUS(sendSignalingMessage(pSampleStreamingSession, pSignalingMessage));
    #endif
    #endif

#ifdef ENABLE_DATA_CHANNEL
    PRtcDataChannel pDataChannel = NULL;
    PRtcPeerConnection pPeerConnection = pSampleStreamingSession->pPeerConnection;
    SIZE_T datachannelLocalOpenCount = 0;

    CHK_STATUS(createDataChannel(pPeerConnection, pChannelName, NULL, &pDataChannel));
    DLOGI("[KVS Viewer] Creating data channel...completed");

    CHK_STATUS(dataChannelOnOpen(pDataChannel, (UINT64) &datachannelLocalOpenCount, dataChannelOnOpenCallback));
    DLOGI("[KVS Viewer] Data Channel open now...");
#endif

    while (!ATOMIC_LOAD_BOOL(&pSampleConfiguration->interrupted) && !ATOMIC_LOAD_BOOL(&pSampleStreamingSession->terminateFlag)) {
        THREAD_SLEEP(HUNDREDS_OF_NANOS_IN_A_SECOND);
    }

CleanUp:

#if 1
    SAFE_MEMFREE(pOfferSessionDescriptionInit);
    pOfferSessionDescriptionInit = NULL;
    SAFE_MEMFREE(pSignalingMessage);
    pSignalingMessage = NULL;
#endif
    if (retStatus != STATUS_SUCCESS) {
        DLOGE("[KVS Viewer] Terminated with status code 0x%08x", retStatus);
    }

    DLOGI("[KVS Viewer] Cleaning up....");

    if (locked) {
        MUTEX_UNLOCK(pSampleConfiguration->sampleConfigurationObjLock);
    }

    if (pSampleConfiguration->enableFileLogging) {
        freeFileLogger();
    }
    if (pSampleConfiguration != NULL) {
        retStatus = freeSignalingClient(&pSampleConfiguration->signalingClientHandle);
        if (retStatus != STATUS_SUCCESS) {
            DLOGE("[KVS Viewer] freeSignalingClient(): operation returned status code: 0x%08x ", retStatus);
        }

        retStatus = freeSampleConfiguration(&pSampleConfiguration);
        if (retStatus != STATUS_SUCCESS) {
            DLOGE("[KVS Viewer] freeSampleConfiguration(): operation returned status code: 0x%08x ", retStatus);
        }
    }
    DLOGI("[KVS Viewer] Cleanup done");

    RESET_INSTRUMENTED_ALLOCATORS();

    return STATUS_FAILED(retStatus) ? EXIT_FAILURE : EXIT_SUCCESS;
}
