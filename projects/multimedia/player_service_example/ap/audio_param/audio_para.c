
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include <components/audio_param_ctrl.h>
#include <components/bk_audio/audio_algorithms/aec_v3_algorithm_v2.h>

// hardware speaker has two version, the new black speaker box is set to 1, else set  HARDWARE_SPEAKER_VER to 0 in kconfig.projbuild
/// customer eq parameter
#define EQTotalNum 5
#define EQGAIN 13014

//E0_freq_120_gain_4_qval_0.8_type_1_LS
#define EQ0 1
#define EQ0A0 -2041723
#define EQ0A1 994948
#define EQ0B0 1055782
#define EQ0B1 -2041195
#define EQ0B2 988268
#define EQ0FREQ 0x42f00000
#define EQ0GAIN 0x40800000
#define EQ0QVAL 0x3f4ccccd
#define EQ0FTYPE 0x01

//E1_freq_280_gain_n15_qval_1_type_0_PK
#define EQ1 1
#define EQ1A0 -1844750
#define EQ1A1 807357
#define EQ1B0 949414
#define EQ1B1 -1844750
#define EQ1B2 906518
#define EQ1FREQ 0x438bd99a
#define EQ1GAIN 0xc1700000
#define EQ1QVAL 0x3f800000
#define EQ1FTYPE 0x00

//E2_freq_1380_gain_0.9_qval_1_type_0_PK
#define EQ2 1
#define EQ2A0 -1443102
#define EQ2A1 635993
#define EQ2B0 1071097
#define EQ2B1 -1443102
#define EQ2B2 613471
#define EQ2FREQ 0x44ac8948
#define EQ2GAIN 0x3f666666
#define EQ2QVAL 0x3f800000
#define EQ2FTYPE 0x00

//E3_freq_1682_gain_n15_qval_0.7_type_2_HS
#define EQ3 1
#define EQ3A0 -1457418
#define EQ3A1 560860
#define EQ3B0 277181
#define EQ3B1 -196677
#define EQ3B2 71513
#define EQ3FREQ 0x44d244cd
#define EQ3GAIN 0xc1700000
#define EQ3QVAL 0x3f333333
#define EQ3FTYPE 0x02

//E4_freq_6000_gain_n12_qval_0.7_type_2_HS
#define EQ4 1
#define EQ4A0 633202
#define EQ4A1 243822
#define EQ4B0 724461
#define EQ4B1 880009
#define EQ4B2 321129
#define EQ4FREQ 0x45bb8000
#define EQ4GAIN 0xc1400000
#define EQ4QVAL 0x3f333333
#define EQ4FTYPE 0x02

#define EQSAMP   0x3e80
#define EQFGAIN  0xc0000000

#define FILTER_PREGAIN_FRA_BITS (14)

#define CUST_EQ_PARA_DL_VOICE()                 \
{                                               \
    .app_eq_en = 1,                             \
    .eq_en = 1,                                 \
    .filters = EQTotalNum,                      \
    .globle_gain = EQGAIN,                      \
    .eq_para[0].a[0] = -EQ0A0,                  \
    .eq_para[0].a[1] = -EQ0A1,                  \
    .eq_para[0].b[0] = EQ0B0,                   \
    .eq_para[0].b[1] = EQ0B1,                   \
    .eq_para[0].b[2] = EQ0B2,                   \
    .eq_para[1].a[0] = -EQ1A0,                  \
    .eq_para[1].a[1] = -EQ1A1,                  \
    .eq_para[1].b[0] = EQ1B0,                   \
    .eq_para[1].b[1] = EQ1B1,                   \
    .eq_para[1].b[2] = EQ1B2,                   \
    .eq_para[2].a[0] = -EQ2A0,                  \
    .eq_para[2].a[1] = -EQ2A1,                  \
    .eq_para[2].b[0] = EQ2B0,                   \
    .eq_para[2].b[1] = EQ2B1,                   \
    .eq_para[2].b[2] = EQ2B2,                   \
    .eq_para[3].a[0] = -EQ3A0,                  \
    .eq_para[3].a[1] = -EQ3A1,                  \
    .eq_para[3].b[0] = EQ3B0,                   \
    .eq_para[3].b[1] = EQ3B1,                   \
    .eq_para[3].b[2] = EQ3B2,                   \
    .eq_para[4].a[0] = -EQ4A0,                  \
    .eq_para[4].a[1] = -EQ4A1,                  \
    .eq_para[4].b[0] = EQ4B0,                   \
    .eq_para[4].b[1] = EQ4B1,                   \
    .eq_para[4].b[2] = EQ4B2,                   \
    .eq_load.f_gain     = EQFGAIN,              \
    .eq_load.samplerate = EQSAMP,               \
    .eq_load.eq_load_para[0].freq   = EQ0FREQ,  \
    .eq_load.eq_load_para[0].gain   = EQ0GAIN,  \
    .eq_load.eq_load_para[0].q_val  = EQ0QVAL,  \
    .eq_load.eq_load_para[0].type   = EQ0FTYPE, \
    .eq_load.eq_load_para[0].enable  = EQ0,     \
    .eq_load.eq_load_para[1].freq   = EQ1FREQ,  \
    .eq_load.eq_load_para[1].gain   = EQ1GAIN,  \
    .eq_load.eq_load_para[1].q_val  = EQ1QVAL,  \
    .eq_load.eq_load_para[1].type   = EQ1FTYPE, \
    .eq_load.eq_load_para[1].enable  = EQ1,     \
    .eq_load.eq_load_para[2].freq   = EQ2FREQ,  \
    .eq_load.eq_load_para[2].gain   = EQ2GAIN,  \
    .eq_load.eq_load_para[2].q_val  = EQ2QVAL,  \
    .eq_load.eq_load_para[2].type   = EQ2FTYPE, \
    .eq_load.eq_load_para[2].enable  = EQ2,     \
    .eq_load.eq_load_para[3].freq   = EQ3FREQ,  \
    .eq_load.eq_load_para[3].gain   = EQ3GAIN,  \
    .eq_load.eq_load_para[3].q_val  = EQ3QVAL,  \
    .eq_load.eq_load_para[3].type   = EQ3FTYPE, \
    .eq_load.eq_load_para[3].enable  = EQ3,     \
    .eq_load.eq_load_para[4].freq   = EQ4FREQ,  \
    .eq_load.eq_load_para[4].gain   = EQ4GAIN,  \
    .eq_load.eq_load_para[4].q_val  = EQ4QVAL,  \
    .eq_load.eq_load_para[4].type   = EQ4FTYPE, \
    .eq_load.eq_load_para[4].enable  = EQ4,     \
}

#define CUST_EQ_PARA_UL_VOICE()                 \
{                                               \
    .app_eq_en = 0,                             \
    .eq_en = 1,                                 \
    .filters = EQTotalNum,                      \
    .globle_gain = EQGAIN,                      \
    .eq_para[0].a[0] = -EQ0A0,                  \
    .eq_para[0].a[1] = -EQ0A1,                  \
    .eq_para[0].b[0] = EQ0B0,                   \
    .eq_para[0].b[1] = EQ0B1,                   \
    .eq_para[0].b[2] = EQ0B2,                   \
    .eq_para[1].a[0] = -EQ1A0,                  \
    .eq_para[1].a[1] = -EQ1A1,                  \
    .eq_para[1].b[0] = EQ1B0,                   \
    .eq_para[1].b[1] = EQ1B1,                   \
    .eq_para[1].b[2] = EQ1B2,                   \
    .eq_para[2].a[0] = -EQ2A0,                  \
    .eq_para[2].a[1] = -EQ2A1,                  \
    .eq_para[2].b[0] = EQ2B0,                   \
    .eq_para[2].b[1] = EQ2B1,                   \
    .eq_para[2].b[2] = EQ2B2,                   \
    .eq_para[3].a[0] = -EQ3A0,                  \
    .eq_para[3].a[1] = -EQ3A1,                  \
    .eq_para[3].b[0] = EQ3B0,                   \
    .eq_para[3].b[1] = EQ3B1,                   \
    .eq_para[3].b[2] = EQ3B2,                   \
    .eq_para[4].a[0] = -EQ4A0,                  \
    .eq_para[4].a[1] = -EQ4A1,                  \
    .eq_para[4].b[0] = EQ4B0,                   \
    .eq_para[4].b[1] = EQ4B1,                   \
    .eq_para[4].b[2] = EQ4B2,                   \
    .eq_load.f_gain     = EQFGAIN,              \
    .eq_load.samplerate = EQSAMP,               \
    .eq_load.eq_load_para[0].freq   = EQ0FREQ,  \
    .eq_load.eq_load_para[0].gain   = EQ0GAIN,  \
    .eq_load.eq_load_para[0].q_val  = EQ0QVAL,  \
    .eq_load.eq_load_para[0].type   = EQ0FTYPE, \
    .eq_load.eq_load_para[0].enable  = EQ0,     \
    .eq_load.eq_load_para[1].freq   = EQ1FREQ,  \
    .eq_load.eq_load_para[1].gain   = EQ1GAIN,  \
    .eq_load.eq_load_para[1].q_val  = EQ1QVAL,  \
    .eq_load.eq_load_para[1].type   = EQ1FTYPE, \
    .eq_load.eq_load_para[1].enable  = EQ1,     \
    .eq_load.eq_load_para[2].freq   = EQ2FREQ,  \
    .eq_load.eq_load_para[2].gain   = EQ2GAIN,  \
    .eq_load.eq_load_para[2].q_val  = EQ2QVAL,  \
    .eq_load.eq_load_para[2].type   = EQ2FTYPE, \
    .eq_load.eq_load_para[2].enable  = EQ2,     \
    .eq_load.eq_load_para[3].freq   = EQ3FREQ,  \
    .eq_load.eq_load_para[3].gain   = EQ3GAIN,  \
    .eq_load.eq_load_para[3].q_val  = EQ3QVAL,  \
    .eq_load.eq_load_para[3].type   = EQ3FTYPE, \
    .eq_load.eq_load_para[3].enable  = EQ3,     \
    .eq_load.eq_load_para[4].freq   = EQ4FREQ,  \
    .eq_load.eq_load_para[4].gain   = EQ4GAIN,  \
    .eq_load.eq_load_para[4].q_val  = EQ4QVAL,  \
    .eq_load.eq_load_para[4].type   = EQ4FTYPE, \
    .eq_load.eq_load_para[4].enable  = EQ4,     \
}



#define CUST_AEC_CONFIG_VOICE()                                          \
{                                                                        \
    .app_aec_en = 1,                                                     \
    .aec_enable = 1,                                                     \
    .init_flags = 0x1f,                                                  \
    .ec_filter = 0x7,                                                    \
    .ec_depth = 0x2,                                                     \
    .mic_delay = 16,                                                     \
    .drc_gain = 0,                                                       \
    .voice_vol = 0xe,                                                    \
    .ref_scale = 0,                                                      \
    .ns_level = 0x5,                                                     \
    .ns_para = 0x2,                                                      \
    .ns_filter = 0x7,                                                    \
    .ns_type = NS_TRADITION,                                             \
    .vad_enable = 0,                                                     \
    .vad_start_threshold = 480,                                          \
    .vad_stop_threshold = 960,                                           \
    .vad_silence_threshold = 320,                                        \
    .vad_eng_threshold =2000,                                            \
    .dual_mic_enable = 0,                                                \
    .dual_mic_distance = 21,                                             \
}

#define CUST_SYS_CONFIG_VOICE()                                          \
{                                                                        \
    .app_sys_en = 1,                                                     \
    .mic0_digital_gain=16,                                               \
    .mic0_analog_gain =20,                                               \
    .mic1_digital_gain=15,                                               \
    .mic1_analog_gain =18,                                               \
    .mic2_digital_gain=14,                                               \
    .mic2_analog_gain =16,                                               \
    .spk0_digital_gain = -10,                                            \
    .spk0_analog_gain  = 4,                                              \
}

/* HW DRC. Preset=OFF, mode=2 */
#define CUST_DRC_CONFIG_SPK()                                                                      \
{                                                                                                  \
    .app_drc_en             = 1,                                                                   \
    .param = {                                                                                     \
        .preset                 = 0,                                                               \
        .mode                   = 2,                                                               \
        .low_boost_db_x10       = 80,                                                              \
        .threshold_dbfs_x10     = -200,                                                            \
        .compress_strength_x100 = 85,                                                              \
    .k_val                  = { 0x493, 0x2B1, 0x1B4, 0x152, 0xD9, 0xD0, 0x7F, 0xE3 },              \
    .p_reg                  = { 0x1, 0x4, 0xF, 0x26, 0x39, 0x4E, 0x6A },                           \
    .st_val                 = { -218, 123477, 380602, 755047, 1936952, 2064113, 3678449, 950272 }, \
    },                                                                                             \
}

app_aud_para_t app_aud_cust_voice_para = {
    .service_type   = AUD_SERVICE_DOORBELL_VOC,
    .sys_config     = CUST_SYS_CONFIG_VOICE(),
    .aec_v3_config  = CUST_AEC_CONFIG_VOICE(),
    .eq_dl_config   = CUST_EQ_PARA_DL_VOICE(),
    .drc_config     = CUST_DRC_CONFIG_SPK(),
};

app_aud_para_t app_aud_cust_single_spk_para = {
    .service_type    = AUD_SERVICE_SINGLE_SPK,
    .sys_config      = CUST_SYS_CONFIG_VOICE(),
    .drc_config      = CUST_DRC_CONFIG_SPK(),
};


app_aud_para_t * get_app_aud_cust_para(app_aud_service_type_t service_type)
{
    switch (service_type)
    {
        case AUD_SERVICE_DOORBELL_VOC:
        case AUD_SERVICE_AI_VOC:
            app_aud_cust_voice_para.service_type = service_type;
            return &app_aud_cust_voice_para;

        case AUD_SERVICE_SINGLE_SPK:
            app_aud_cust_single_spk_para.service_type = service_type;
            return &app_aud_cust_single_spk_para;
        default:
            return NULL;
    }
}
