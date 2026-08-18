/**
 * @file hfp_ag_demo.h
 *
 * HFP AG (Audio Gateway) demo - control plane. Thin application policy on top
 * of the componentized bk_hfp_ag_service.
 */

#ifndef HFP_AG_DEMO_H
#define HFP_AG_DEMO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    HFP_AG_DEBUG_LEVEL_ERROR,
    HFP_AG_DEBUG_LEVEL_WARNING,
    HFP_AG_DEBUG_LEVEL_INFO,
    HFP_AG_DEBUG_LEVEL_DEBUG,
    HFP_AG_DEBUG_LEVEL_VERBOSE,
};

#define HFP_AG_DEBUG_LEVEL HFP_AG_DEBUG_LEVEL_INFO

#define LOGE(format, ...) do{if(HFP_AG_DEBUG_LEVEL >= HFP_AG_DEBUG_LEVEL_ERROR)   BK_LOGE(LOG_TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGW(format, ...) do{if(HFP_AG_DEBUG_LEVEL >= HFP_AG_DEBUG_LEVEL_WARNING) BK_LOGW(LOG_TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGI(format, ...) do{if(HFP_AG_DEBUG_LEVEL >= HFP_AG_DEBUG_LEVEL_INFO)    BK_LOGI(LOG_TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGD(format, ...) do{if(HFP_AG_DEBUG_LEVEL >= HFP_AG_DEBUG_LEVEL_DEBUG)   BK_LOGD(LOG_TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGV(format, ...) do{if(HFP_AG_DEBUG_LEVEL >= HFP_AG_DEBUG_LEVEL_VERBOSE) BK_LOGV(LOG_TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)


/**
 * @brief Initialize the HFP AG demo (registers policy callback + inits the AG service).
 * @return 0 on success, negative on failure.
 */
int hfp_ag_demo_init(void);

/* CLI helpers (also callable directly). */
void hfp_ag_demo_connect(const uint8_t *addr);
void hfp_ag_demo_disconnect(void);
void hfp_ag_demo_incoming_call(const char *number);   /* simulate an incoming call */
void hfp_ag_demo_dial_out(const char *number);        /* simulate an AG-initiated call */
void hfp_ag_demo_answer(void);                        /* mark the call active */
void hfp_ag_demo_hangup(void);                        /* end / reject the call */
void hfp_ag_demo_audio(uint8_t connect);              /* set up / tear down SCO */
void hfp_ag_demo_set_codec(uint8_t msbc);            /* choose SCO codec: 1 = mSBC, 0 = CVSD */
void hfp_ag_demo_send_vgs(uint8_t vgs);                /* set speaker volume */
void hfp_ag_demo_send_vgm(uint8_t vgm);                /* set microphone volume */
void hfp_ag_demo_set_battery(uint8_t level);          /* report AG battery level (0-5) to HF */
void hfp_ag_demo_custom_cmd(const char *atcmd);       /* send raw result code */
void hfp_ag_demo_switch_role(uint8_t master);         /* switch BT role: 1 = master, 0 = slave */

/** Register the demo CLI commands. */
int cli_hfp_ag_demo_init(void);

#ifdef __cplusplus
}
#endif

#endif /* HFP_AG_DEMO_H */
