#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <components/bk_voice_service.h>
#include <components/bk_voice_service_types.h>
#include <components/bk_voice_read_service.h>
#include <components/bk_voice_read_service_types.h>
#include <components/bk_voice_write_service.h>
#include <components/bk_voice_write_service_types.h>

#if CONFIG_CLI
#include "cli.h"
#endif

#define TAG "db_uac"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

static voice_handle_t s_voice;
static voice_read_handle_t s_read;
static voice_write_handle_t s_write;
static beken_thread_t s_writer_thread;
static volatile bool s_writer_running;

static int db_uac_loopback_cb(unsigned char *data, unsigned int len, void *args)
{
    (void)args;
    (void)data;
    return len;
}

static void db_uac_writer_main(void *arg)
{
    (void)arg;
    uint8_t frame[320];
    uint32_t seq = 0;

    os_memset(frame, 0, sizeof(frame));
    while (s_writer_running) {
        /* 8 kHz, 16-bit, mono, 20 ms: 320 bytes. This simulates the doorbell
         * downlink path (WIFI_RX -> RAW_WRITE -> UAC_SPK) without requiring a
         * phone client to provide audio frames. */
        frame[0] = (uint8_t)(seq & 0xff);
        frame[1] = (uint8_t)((seq >> 8) & 0xff);
        if (s_write) {
            int ret = bk_voice_write_frame_data(s_write, (char *)frame, sizeof(frame));
            if (ret != BK_OK) {
                LOGE("writer write failed: %d\n", ret);
            }
        }
        seq++;
        rtos_delay_milliseconds(20);
    }

    s_writer_thread = NULL;
    rtos_delete_thread(NULL);
}

static void db_uac_stop(void)
{
    if (s_writer_running) {
        s_writer_running = false;
        while (s_writer_thread) {
            rtos_delay_milliseconds(10);
        }
    }
    if (s_read) {
        bk_voice_read_stop(s_read);
    }
    if (s_write) {
        bk_voice_write_stop(s_write);
    }
    if (s_voice) {
        bk_voice_stop(s_voice);
    }
    if (s_read) {
        bk_voice_read_deinit(s_read);
        s_read = NULL;
    }
    if (s_write) {
        bk_voice_write_deinit(s_write);
        s_write = NULL;
    }
    if (s_voice) {
        bk_voice_deinit(s_voice);
        s_voice = NULL;
    }
}

static int db_uac_start(void)
{
    if (s_voice) {
        LOGI("already running\n");
        return 0;
    }

    voice_cfg_t voice_cfg;
    os_memset(&voice_cfg, 0, sizeof(voice_cfg));

    voice_cfg.mic_type = MIC_TYPE_UAC;
    uac_mic_stream_cfg_t mic_cfg = UAC_MIC_STREAM_CFG_DEFAULT();
    mic_cfg.samp_rate = 8000;
    mic_cfg.frame_size = 320;
    mic_cfg.out_block_size = 320;
    mic_cfg.out_block_num = 1;
    voice_cfg.mic_cfg.uac_mic_cfg = mic_cfg;

    voice_cfg.aec_en = true;
    aec_v3_algorithm_cfg_t aec_cfg = DEFAULT_AEC_V3_ALGORITHM_CONFIG();
    aec_cfg.out_block_num = 1;
    aec_cfg.aec_cfg.fs = 8000;
    aec_cfg.aec_cfg.mode = AEC_MODE_SOFTWARE;
    aec_cfg.dual_ch = 0;
    aec_cfg.multi_in_port_num = 1;
    voice_cfg.aec_cfg.aec_alg_cfg = aec_cfg;

    voice_cfg.enc_type = AUDIO_ENC_TYPE_PCM;
    voice_cfg.enc_cfg.pcm_enc_cfg = 0;
    voice_cfg.read_pool_size = 320;

    voice_cfg.dec_type = AUDIO_DEC_TYPE_PCM;
    voice_cfg.dec_cfg.pcm_dec_cfg = 0;
    voice_cfg.write_pool_size = 320;

    voice_cfg.spk_type = SPK_TYPE_UAC;
    uac_speaker_stream_cfg_t spk_cfg = UAC_SPEAKER_STREAM_CFG_DEFAULT();
    spk_cfg.samp_rate = 8000;
    spk_cfg.frame_size = 320;
    spk_cfg.multi_out_port_num = 1;
    voice_cfg.spk_cfg.uac_spk_cfg = spk_cfg;

#if CONFIG_VOICE_SERVICE_EQ
    voice_cfg.eq_en = true;
    eq_algorithm_cfg_t eq_cfg = DEFAULT_EQ_ALGORITHM_CONFIG();
    eq_cfg.eq_chl_num = 1;
    voice_cfg.eq_cfg.eq_alg_cfg = eq_cfg;
#endif

    s_voice = bk_voice_init(&voice_cfg);
    if (!s_voice) {
        LOGE("bk_voice_init failed\n");
        goto fail;
    }

    voice_read_cfg_t read_cfg = VOICE_READ_CFG_DEFAULT();
    read_cfg.voice_handle = s_voice;
    read_cfg.max_read_size = 320;
    read_cfg.voice_read_callback = db_uac_loopback_cb;
    read_cfg.mem_type = AUDIO_MEM_TYPE_PSRAM;
    s_read = bk_voice_read_init(&read_cfg);
    if (!s_read) {
        LOGE("bk_voice_read_init failed\n");
        goto fail;
    }

    voice_write_cfg_t write_cfg = VOICE_WRITE_CFG_DEFAULT();
    write_cfg.voice_handle = s_voice;
    write_cfg.mem_type = AUDIO_MEM_TYPE_PSRAM;
    s_write = bk_voice_write_init(&write_cfg);
    if (!s_write) {
        LOGE("bk_voice_write_init failed\n");
        goto fail;
    }

    if (bk_voice_start(s_voice) != BK_OK) {
        LOGE("bk_voice_start failed\n");
        goto fail;
    }
    if (bk_voice_read_start(s_read) != BK_OK) {
        LOGE("bk_voice_read_start failed\n");
        goto fail;
    }
    if (bk_voice_write_start(s_write) != BK_OK) {
        LOGE("bk_voice_write_start failed\n");
        goto fail;
    }

    s_writer_running = true;
    if (rtos_create_thread(&s_writer_thread,
                           BEKEN_DEFAULT_WORKER_PRIORITY - 1,
                           "db_uac_wr",
                           db_uac_writer_main,
                           2048,
                           NULL) != BK_OK) {
        LOGE("create writer thread failed\n");
        s_writer_running = false;
        goto fail;
    }

    LOGI("doorbell-like UAC voice started (UAC mic+spk, PCM, AEC software, 20ms downlink writer)\n");
    return 0;

fail:
    db_uac_stop();
    return -1;
}

#if CONFIG_CLI
static void cli_db_uac_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;

    if (argc < 2 || os_strcmp(argv[1], "start") == 0) {
        (void)db_uac_start();
    } else if (os_strcmp(argv[1], "stop") == 0) {
        db_uac_stop();
        LOGI("doorbell-like UAC voice stopped\n");
    } else {
        LOGI("usage: db_uac start|stop\n");
    }
}

COMPONENTS_CLI_CMD_EXPORT
static const struct cli_command s_db_uac_cmds[] = {
    {"db_uac", "db_uac start|stop", cli_db_uac_cmd},
};
#endif
