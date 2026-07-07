/**
 * @file pbap_pce_demo.c
 *
 * PBAP PCE demo: registers callback, init/deinit, and handles events.
 * Used together with CLI for bk_dm_pbap_pce.h API functional testing.
 */

#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "pbap_pce_demo_cli.h"
#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_pbap_pce.h"

#define TAG "Demo===>pbap_pce"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define BD_ADDR_STR(bda) (bda)[0], (bda)[1], (bda)[2], (bda)[3], (bda)[4], (bda)[5]

#define VCARD_MAX_NAME_LEN   128
#define VCARD_MAX_TEL_LEN    32
#define VCARD_MAX_DATETIME_LEN 24
#define VCARD_PARSE_TAIL_MAX    256
#define VCARD_PARSE_COMBINED_MAX (BK_PBAP_PCE_DEFAULT_MAX_RECV_SIZE+VCARD_PARSE_TAIL_MAX)

#define CARD_LIST_MAX_HANDLE 64
#define CARD_LIST_MAX_NAME   VCARD_MAX_TEL_LEN
#define CARD_LIST_TAIL_MAX   256
#define CARD_LIST_COMBINED_MAX (BK_PBAP_PCE_DEFAULT_MAX_RECV_SIZE+CARD_LIST_TAIL_MAX)
/* Shared by vcard_list and vcard_parse (never concurrent). */
#define VCARD_TAIL_MAX      256
#define VCARD_COMBINED_MAX  (BK_PBAP_PCE_DEFAULT_MAX_RECV_SIZE+VCARD_TAIL_MAX)

#define PBAP_EVENT_QUEUE_LEN   100
/** Sentinel event: event task exits, drains queue, deinits it, then deletes self. */
#define PBAP_PCE_DEMO_EXIT_EVT ((bk_pbap_pce_cb_event_t)0xFE)
/* Event task stack: handle_event -> vcard_parse (name[128]+tel[32]) or vcard_list_parse_cards,
 * plus many LOGI/bk_printf per item. 10KB to avoid stack overflow (water mark was ~30 with 6KB). */
#define PBAP_EVENT_TASK_STACK  (1024 * 6)

/** Message sent from PBAP callback (BT stack context) to event task. */
typedef struct {
    bk_pbap_pce_cb_event_t event;
    bk_pbap_pce_status_t   status;
    uint8_t                bd_addr[BK_BD_ADDR_LEN];
    uint16_t               data_len;        /* for get_phonebook/vcard_list/vcard_cfm */
    uint8_t               *data_ptr;       /* malloc'd in callback, event task free's after use */
    uint16_t               phonebook_size;  /* for get_size_cfm */
    uint8_t                remote_supported_repositories;
    uint32_t               remote_supported_features;
} pbap_event_msg_t;

static beken_queue_t   s_pbap_event_queue;
static beken_thread_t  s_pbap_event_thread;
static uint16_t        s_pbap_event_queue_count = 0;

/* Shared by vcard_list_parse_cards and vcard_parse_name_and_tel (event task; never concurrent). */
static char s_vcard_tail[VCARD_TAIL_MAX];
static size_t s_vcard_tail_len;
static char s_vcard_combined[VCARD_COMBINED_MAX];
static char s_vcard_name[VCARD_MAX_NAME_LEN];   /* list: name (first 32); parse: name */
static char s_vcard_handle_tel[CARD_LIST_MAX_HANDLE];             /* list: card handle; parse: tel number */
static char s_vcard_call_datetime[VCARD_MAX_DATETIME_LEN];        /* parse: X-IRMC-CALL-DATETIME */
static void vcard_list_parse_cards_reset(void)
{
    s_vcard_tail_len = 0;
}

static void vcard_list_save_tail(const char *from, const char *end)
{
    size_t n = (size_t)(end - from);
    if (n > VCARD_TAIL_MAX) n = VCARD_TAIL_MAX;
    if (n > 0) {
        os_memcpy(s_vcard_tail, from, n);
        s_vcard_tail_len = n;
    }
}

/** Extract one quoted attr (e.g. handle="x" or name="y") from [card, tag_end). Case-insensitive attr name. */
static void vcard_list_extract_attr(const char *card, const char *tag_end,
                                   const char *attr, int attr_len,
                                   char *out, size_t out_size)
{
    const char *q = card;
    out[0] = '\0';
    while ((size_t)(tag_end - q) >= (size_t)(attr_len + 1)) {
        int match = 1;
        for (int i = 0; i < attr_len; i++) {
            char c = (char)q[i];
            if (c >= 'A' && c <= 'Z') {
                c += 32;
            }
            char e = (attr[i] >= 'A' && attr[i] <= 'Z') ? (char)(attr[i] + 32) : attr[i];
            if (c != e) {
                match = 0;
                break;
            }
        }
        if (match && (q[attr_len] == '=' || (q[attr_len] == ' ' && (size_t)(tag_end - q) > (size_t)(attr_len + 1) && q[attr_len + 1] == '='))) {
            q += attr_len;
            while (q < tag_end && (*q == ' ' || *q == '\t')) {
                q++;
            }
            if (q < tag_end && *q == '=') {
                q++;
                while (q < tag_end && (*q == ' ' || *q == '\t')) {
                    q++;
                }
                if (q < tag_end && (*q == '"' || *q == '\'')) {
                    char quote = *q++;
                    const char *start = q;
                    while (q < tag_end && *q != quote) {
                        q++;
                    }
                    size_t vl = (size_t)(q - start);
                    if (vl >= out_size) {
                        vl = out_size - 1;
                    }
                    if (vl > 0) {
                        os_memcpy(out, start, vl);
                        out[vl] = '\0';
                    }
                }
            }
            return;
        }
        q++;
    }
}

/**
 * Parse vCard list XML-like body; print each card's handle/name.
 * Incomplete tail is saved and prepended to the next call to avoid drop across chunks.
 * Format: <card handle="908.vcf" name="057123672159"/> (or name then handle).
 * 
 *   <!DTD for the PBAP vCard-Listing Object-->
 *   <!ELEMENT vcard-listing ( card )* >
 *   <!ATTLIST vcard-listing version CDATA #FIXED “1.0”>
 *   <!ELEMENT card EMPTY>
 *   <!ATTLIST card handle CDATA #REQUIRED name CDATA #IMPLIED >
 *   <?xml version=“1.0”?>
 *   <!DOCTYPE vcard-listing SYSTEM “vcard-listing.dtd”>
 *   <vCard-listing version=“1.0”>
 *   <card handle = ”0.vcf” name = ”Miyajima;Andy”/>
 *   <card handle = ”1.vcf” name = ”Poujade;Guillaume”/>
 *   <card handle = ”2.vcf” name = ”Hung;Scott”/>
 *   <card handle = ”3.vcf” name = ”Afonso;Arthur”/>
 *   <card handle = ”6.vcf” name = ”McHardy;Jamie”/>
 *   <card handle = ”7.vcf” name = ”Toropov;Dmitri”/>
 *   <card handle = ”10.vcf” name = ”Weinans;Erwin”/>
 *   </vCard-listing
 */
static void vcard_list_parse_cards(const char *data, size_t len)
{
    const char *parse_base;
    size_t parse_len;

    if (s_vcard_tail_len == 0 && (!data || len == 0)) return;
    if (s_vcard_tail_len > 0) {
        size_t ct = s_vcard_tail_len;
        size_t cd = data ? len : 0;
        if (ct + cd > VCARD_COMBINED_MAX) {
            if (ct >= VCARD_COMBINED_MAX) { ct = 0; cd = (len > VCARD_COMBINED_MAX) ? VCARD_COMBINED_MAX : len; }
            else { cd = VCARD_COMBINED_MAX - ct; }
        }
        os_memcpy(s_vcard_combined, s_vcard_tail, ct);
        if (cd && data) os_memcpy(s_vcard_combined + ct, data, cd);
        parse_base = s_vcard_combined;
        parse_len  = ct + cd;
        s_vcard_tail_len = 0;
    } else {
        parse_base = data;
        parse_len  = len;
    }

    const char *p   = parse_base;
    const char *end = parse_base + parse_len;
    uint8_t print_count = 3;
    while (p < end) {
        const char *card = p;
        while (card < end && (size_t)(end - card) >= 5 &&
               (card[0] != '<' || card[1] != 'c' || card[2] != 'a' || card[3] != 'r' || card[4] != 'd')) {
            card++;
        }
        if (card >= end || (size_t)(end - card) < 5) {
            vcard_list_save_tail(card, end);
            break;
        }
        p = card + 5;
        while (p < end && *p != '>' && *p != '/') {
            p++;
        }
        if (p >= end) {
            vcard_list_save_tail(card, end);
            break;
        }
        const char *tag_end = p;

        os_memset(s_vcard_handle_tel, 0, sizeof(s_vcard_handle_tel));
        os_memset(s_vcard_name, 0, CARD_LIST_MAX_NAME);
        vcard_list_extract_attr(card, tag_end, "handle", 6, s_vcard_handle_tel, CARD_LIST_MAX_HANDLE);
        vcard_list_extract_attr(card, tag_end, "name", 4, s_vcard_name, CARD_LIST_MAX_NAME);
        if (print_count > 0) {
            LOGI("%s - %s\n", s_vcard_name[0] ? s_vcard_name : "(none)", s_vcard_handle_tel[0] ? s_vcard_handle_tel : "(none)");
            print_count--;
            if (print_count == 0) {
                LOGI("......\n");
            }
        }

        p = tag_end;
        if (p < end && *p == '/') {
            p++;
        }
        if (p < end && *p == '>') {
            p++;
        }
    }
    /* Only print last card when we had >3 cards and skipped it in the loop. */
    LOGI("%s - %s\n", s_vcard_name[0] ? s_vcard_name : "(none)", s_vcard_handle_tel[0] ? s_vcard_handle_tel : "(none)");
}

static void vcard_parse_name_and_tel_reset(void)
{
    s_vcard_tail_len = 0;
    s_vcard_name[0]  = '\0';
    s_vcard_handle_tel[0]   = '\0';
    s_vcard_call_datetime[0] = '\0';
}

static int vcard_type_eq(const char *p, size_t len, const char *type, size_t tlen)
{
    if (len < tlen) return 0;
    for (size_t i = 0; i < tlen; i++) {
        if ((p[i] | 0x20) != (type[i] | 0x20)) return 0;
    }
    return (len == tlen || p[tlen] == ';' || p[tlen] == ':');
}

/* 1 if type (p, len) contains "irmc-call-datetime" (case-insensitive) */
static int vcard_type_has_irmc_call_datetime(const char *p, size_t len)
{
    const char *key = "irmc-call-datetime";
    size_t klen = 18;
    if (len < klen) return 0;
    for (size_t i = 0; i <= len - klen; i++) {
        size_t j;
        for (j = 0; j < klen && (p[i + j] | 0x20) == key[j]; j++) ;
        if (j == klen) return 1;
    }
    return 0;
}

static void vcard_copy_val(char *dst, size_t max, const char *val, size_t val_len)
{
    if (val_len >= max) val_len = max - 1;
    if (val_len > 0) {
        os_memcpy(dst, val, val_len);
        dst[val_len] = '\0';
    }
}

/** Process one vCard line [line_start, line_end). Fills name/aux/datetime; returns updated count. */
static unsigned vcard_parse_one_line(const char *line_start, const char *line_end, unsigned count,
                                     char *name, size_t name_sz, char *aux, size_t aux_sz,
                                     char *datetime, size_t datetime_sz)
{
    const char *colon = line_start;
    while (colon < line_end && *colon != ':') colon++;
    if (colon >= line_end || colon == line_start) return count;
    const char *type_start = line_start;
    while (type_start < colon && (*type_start == ' ' || *type_start == '\t')) type_start++;
    size_t type_len = (size_t)(colon - type_start);
    const char *val = colon + 1;
    const char *val_end = line_end;
    while (val_end > val && (val_end[-1] == ' ' || val_end[-1] == '\t' || val_end[-1] == '\r' || val_end[-1] == '\n')) val_end--;
    size_t val_len  = (size_t)(val_end - val);

    if (vcard_type_eq(type_start, type_len, "FN", 2)) {
        vcard_copy_val(name, name_sz, val, val_len);
    } else if (vcard_type_eq(type_start, type_len, "N", 1) && name && name[0] == '\0') {
        vcard_copy_val(name, name_sz, val, val_len);
    } else if (vcard_type_eq(type_start, type_len, "TEL", 3)) {
        vcard_copy_val(aux, aux_sz, val, val_len);
    } else if (vcard_type_eq(type_start, type_len, "X-IRMC-CALL-DATETIME", 19)) {
        if (datetime && datetime_sz > 0)
            vcard_copy_val(datetime, datetime_sz, val, val_len);
    } else if (datetime && datetime_sz > 0 && val_len > 0 && vcard_type_has_irmc_call_datetime(type_start, type_len)) {
        vcard_copy_val(datetime, datetime_sz, val, val_len);
    } else if (datetime && datetime_sz > 0 && val_len > 0 &&
               ((type_len >= 6 && (vcard_type_eq(type_start, type_len, "DIALED", 6) || vcard_type_eq(type_start, type_len, "MISSED", 6))) ||
                (type_len >= 8 && vcard_type_eq(type_start, type_len, "RECEIVED", 8)))) {
        /* Fallback: line split across chunks may give "DIALED:20260104T192827" alone */
        vcard_copy_val(datetime, datetime_sz, val, val_len);
    } else if (vcard_type_eq(type_start, type_len, "END", 3)) {
        return count + 1;  /* caller prints name/aux/datetime and clears */
    }
    return count;
}

/** Parse vCard text; incomplete line at chunk end is carried to next call. 
* 
* Main phone book vCard Example :
* BEGIN:VCARD
* VERSION:2.1
* FN:Jean Dupont
* N:Dupont;Jean
* ADR;WORK;QUOTED-PRINTABLE:;Paris 75010;91 Rue du Faubourg SaintMartin
* TEL;CELL;PREF:+1234 56789
* EMAIL;INTERNET:jean.dupont@example.com
* X-BT-UID:A1A2A3A4B1B2C1C2D1D2E1E2E3E4E5E6
* END:VCARD
*
* The time of each call found in och, ich, mch and cch folder:
* X-IRMC-CALL-DATETIME;DIALED:20260104T192958
* X-IRMC-CALL-DATETIME;MISSED:20260104T192958
* X-IRMC-CALL-DATETIME;RECEIVED:20260104T192958
*/
static uint16_t vcard_parse_name_and_tel(const char *data, size_t len, uint16_t last_count)
{
    const char *base;
    size_t plen;
    unsigned count = (unsigned)last_count;
    uint8_t print_count = 3;

    if (s_vcard_tail_len == 0 && (!data || len == 0)) return (uint16_t)count;
    if (s_vcard_tail_len > 0) {
        size_t ct = s_vcard_tail_len;
        size_t cd = data ? len : 0;
        if (ct + cd > VCARD_COMBINED_MAX) {
            if (ct >= VCARD_COMBINED_MAX) { ct = 0; cd = (len > VCARD_COMBINED_MAX) ? VCARD_COMBINED_MAX : len; }
            else { cd = VCARD_COMBINED_MAX - ct; }
        }
        os_memcpy(s_vcard_combined, s_vcard_tail, ct);
        if (cd && data) os_memcpy(s_vcard_combined + ct, data, cd);
        base = s_vcard_combined;
        plen = ct + cd;
        s_vcard_tail_len = 0;
    } else {
        base = data;
        plen = len;
    }

    const char *p = base, *end = base + plen;
    while (p < end) {
        const char *le = p;
        while (le < end && *le != '\r' && *le != '\n') le++;
        if (le >= end && le > p) {
            size_t n = (size_t)(end - p);
            if (n > VCARD_TAIL_MAX) n = VCARD_TAIL_MAX;
            os_memcpy(s_vcard_tail, p, n);
            s_vcard_tail_len = n;
            break;
        }
        if (le > p) {
            unsigned new_count = vcard_parse_one_line(p, le, count,
                                                      s_vcard_name, VCARD_MAX_NAME_LEN,
                                                      s_vcard_handle_tel, sizeof(s_vcard_handle_tel),
                                                      s_vcard_call_datetime, sizeof(s_vcard_call_datetime)
                                                      );
            if (new_count != count) {
                if (print_count > 0) {
                    LOGI(" [%u] , %s - %s - %s\n", new_count,
                              s_vcard_name[0] ? s_vcard_name : "(none)",
                              s_vcard_handle_tel[0] ? s_vcard_handle_tel : "(none)",
                              s_vcard_call_datetime[0] ? s_vcard_call_datetime : "");
                    print_count--;
                } else {
                    LOGI(".....\n");
                }
                s_vcard_name[0] = s_vcard_handle_tel[0] = s_vcard_call_datetime[0] = '\0';
            }
            count = new_count;
        }
        p = le;
        while (p < end && (*p == '\r' || *p == '\n')) p++;
    }
    return (uint16_t)count;
}

static const char *pbap_status_str(bk_pbap_pce_status_t s)
{
    switch (s) {
    case BK_PBAP_PCE_SUCCESS:          return "SUCCESS";
    case BK_PBAP_PCE_FAILURE:          return "FAILURE";
    case BK_PBAP_PCE_BUSY:             return "BUSY";
    case BK_PBAP_PCE_NO_RESOURCE:      return "NO_RESOURCE";
    case BK_PBAP_PCE_NEED_INIT:        return "NEED_INIT";
    case BK_PBAP_PCE_NEED_DEINIT:      return "NEED_DEINIT";
    case BK_PBAP_PCE_NO_CONNECTION:    return "NO_CONNECTION";
    case BK_PBAP_PCE_INVALID_STATE:    return "INVALID_STATE";
    case BK_PBAP_PCE_INVALID_PARAM:    return "INVALID_PARAM";
    case BK_PBAP_PCE_CONTINUE:         return "CONTINUE";
    case BK_PBAP_PCE_BAD_REQUEST:      return "BAD_REQUEST";
    case BK_PBAP_PCE_UNAUTHORIZED:     return "UNAUTHORIZED";
    case BK_PBAP_PCE_NOT_FOUND:        return "NOT_FOUND";
    case BK_PBAP_PCE_FORBIDDEN:        return "FORBIDDEN";
    case BK_PBAP_PCE_IDLE_TIMEOUT:     return "IDLE_TIMEOUT";
    default:                           return "UNKNOWN";
    }
}

/** Process one PBAP event in event task context (avoids blocking BT stack). */
static void pbap_pce_handle_event(pbap_event_msg_t *msg)
{
    const uint8_t *bd_addr = msg->bd_addr;
    const char *status_str = pbap_status_str(msg->status);

    if (!msg) {
        return;
    }

    switch (msg->event) {
    case BK_PBAP_PCE_INIT_EVT:
        LOGI("PBAP PCE INIT_EVT: status=%s\n", status_str);
        break;

    case BK_PBAP_PCE_DEINIT_EVT:
        LOGI("PBAP PCE DEINIT_EVT: status=%s\n", status_str);
        break;

    case BK_PBAP_PCE_CONNECT_CFM_EVT:
        LOGI("PBAP PCE CONNECT_CFM_EVT: status=%s addr=%02x:%02x:%02x:%02x:%02x:%02x\n",
             status_str, BD_ADDR_STR(bd_addr));
        if(msg->status == BK_PBAP_PCE_SUCCESS || msg->status == BK_PBAP_PCE_UNAUTHORIZED)
        {
            pbap_pce_demo_cli_set_connection_state(bd_addr, 1);
            LOGI("  remote_supported_repositories=%u remote_supported_features=%08x\n",
                 msg->remote_supported_repositories, msg->remote_supported_features);
            LOGI(" Supported Repositories: local_phonebook %d, sim %d, speed_dial %d, favorites %d\n",
                 (msg->remote_supported_repositories & BK_PBAP_PCE_REPO_LOCAL_PHONEBOOK) ? 1 : 0,
                 (msg->remote_supported_repositories & BK_PBAP_PCE_REPO_SIM) ? 1 : 0,
                 (msg->remote_supported_repositories & BK_PBAP_PCE_REPO_SPEED_DIAL) ? 1 : 0,
                 (msg->remote_supported_repositories & BK_PBAP_PCE_REPO_FAVORITES) ? 1 : 0);
            LOGI(" Supported Features: download %d, browsing %d, database_identifier %d, folder_version_counters %d, vcard_selecting %d, enhanced_missed_calls %d, x_bt_uci_vcard %d, x_bt_uid_vcard %d, contact_referencing %d, default_contact_image %d\n",
                 (msg->remote_supported_features & BK_PBAP_PCE_FEAT_DOWNLOAD) ? 1 : 0,
                 (msg->remote_supported_features & BK_PBAP_PCE_FEAT_BROWSING) ? 1 : 0,
                 (msg->remote_supported_features & BK_PBAP_PCE_FEAT_DATABASE_IDENTIFIER) ? 1 : 0,
                 (msg->remote_supported_features & BK_PBAP_PCE_FEAT_FOLDER_VERSION_COUNTERS) ? 1 : 0,
                 (msg->remote_supported_features & BK_PBAP_PCE_FEAT_VCARD_SELECTING) ? 1 : 0,
                 (msg->remote_supported_features & BK_PBAP_PCE_FEAT_ENHANCED_MISSED_CALLS) ? 1 : 0,
                 (msg->remote_supported_features & BK_PBAP_PCE_FEAT_X_BT_UCI_VCARD) ? 1 : 0,
                 (msg->remote_supported_features & BK_PBAP_PCE_FEAT_X_BT_UID_VCARD) ? 1 : 0,
                 (msg->remote_supported_features & BK_PBAP_PCE_FEAT_CONTACT_REFERENCING) ? 1 : 0,
                 (msg->remote_supported_features & BK_PBAP_PCE_FEAT_DEFAULT_CONTACT_IMAGE) ? 1 : 0);
        }
        if (msg->status == BK_PBAP_PCE_UNAUTHORIZED) {
            LOGI("  -> Use 'pbap auth <addr> <pin>' to send OBEX auth response\n");
        }
        if(msg->status != BK_PBAP_PCE_SUCCESS && msg->status != BK_PBAP_PCE_UNAUTHORIZED)
        {
            pbap_pce_demo_cli_set_connection_state(bd_addr, 0);
        }
        break;

    case BK_PBAP_PCE_DISCONNECT_CFM_EVT:
        LOGI("PBAP PCE DISCONNECT_CFM_EVT: status=%s addr=%02x:%02x:%02x:%02x:%02x:%02x\n",
             status_str, BD_ADDR_STR(bd_addr));
        pbap_pce_demo_cli_set_connection_state(bd_addr, 0);
        break;

    case BK_PBAP_PCE_GET_PHONEBOOK_CFM_EVT:
        LOGI("PBAP PCE GET_PHONEBOOK_CFM_EVT: status=%s addr=%02x:%02x:%02x:%02x:%02x:%02x data_len=%u\n",
             status_str, BD_ADDR_STR(bd_addr), (unsigned)msg->data_len);
        static uint16_t total_count = 0;
        if (msg->data_len > 0 && msg->data_ptr) {
            LOGI("  body (all %u bytes):\n", (unsigned)msg->data_len);
            // LOGI("%s", (const char *)msg->data_ptr);
            total_count = vcard_parse_name_and_tel((const char *)msg->data_ptr, msg->data_len, total_count);
        }
        if(msg->status != BK_PBAP_PCE_CONTINUE)
        {
            LOGI(" [vCard] total count: %u\n", total_count);
            total_count = 0;
        }
        break;

    case BK_PBAP_PCE_GET_VCARD_LIST_CFM_EVT:
        LOGI("PBAP PCE GET_VCARD_LIST_CFM_EVT: status=%s addr=%02x:%02x:%02x:%02x:%02x:%02x data_len=%u\n",
             status_str, BD_ADDR_STR(bd_addr), (unsigned)msg->data_len);
        if (msg->data_len > 0 && msg->data_ptr) {
            // LOGI("  body (all %u bytes):\n", (unsigned)msg->data_len);
            vcard_list_parse_cards((const char *)msg->data_ptr, msg->data_len);
        }
        break;

    case BK_PBAP_PCE_GET_VCARD_CFM_EVT:
        LOGI("PBAP PCE GET_VCARD_CFM_EVT: status=%s addr=%02x:%02x:%02x:%02x:%02x:%02x data_len=%u\n",
             status_str, BD_ADDR_STR(bd_addr), (unsigned)msg->data_len);
        if (msg->data_len > 0 && msg->data_ptr) {
            LOGI("  body (all %u bytes):\n", (unsigned)msg->data_len);
            // LOGI("%s", (const char *)msg->data_ptr);
            vcard_parse_name_and_tel((const char *)msg->data_ptr, msg->data_len, 0);
        }
        break;

    case BK_PBAP_PCE_SET_PHONEBOOK_CFM_EVT:
        LOGI("PBAP PCE SET_PHONEBOOK_CFM_EVT: status=%s addr=%02x:%02x:%02x:%02x:%02x:%02x\n",
             status_str, BD_ADDR_STR(bd_addr));
        break;

    case BK_PBAP_PCE_GET_SIZE_CFM_EVT:
        LOGI("PBAP PCE GET_SIZE_CFM_EVT: status=%s addr=%02x:%02x:%02x:%02x:%02x:%02x size=%u\n",
             status_str, BD_ADDR_STR(bd_addr), (unsigned)msg->phonebook_size);
        break;

    case BK_PBAP_PCE_ABORT_CFM_EVT:
        LOGI("PBAP PCE ABORT_CFM_EVT: status=%s addr=%02x:%02x:%02x:%02x:%02x:%02x\n",
             status_str, BD_ADDR_STR(bd_addr));
        break;

    default:
        LOGW("PBAP PCE unhandled event: %d\n", (int)msg->event);
        break;
    }
}

/** Signal event task to exit, wait, then clear queue/thread/count handles. */
static void pbap_pce_demo_exit_event_task_and_clear(void)
{
    pbap_event_msg_t exit_msg = {0};
    exit_msg.event = PBAP_PCE_DEMO_EXIT_EVT;
    if (s_pbap_event_queue) {
        (void)rtos_push_to_queue(&s_pbap_event_queue, &exit_msg, BEKEN_WAIT_FOREVER);
        rtos_delay_milliseconds(200);
    }
    s_pbap_event_queue = (beken_queue_t)0;
    s_pbap_event_thread = (beken_thread_t)0;
    s_pbap_event_queue_count = 0;
}

/** Event task: process events from queue so callback returns quickly. On EXIT, drain queue and self-terminate. */
static void pbap_pce_event_task(beken_thread_arg_t arg)
{
    pbap_event_msg_t msg;

    (void)arg;
    while (1) {
        if (rtos_pop_from_queue(&s_pbap_event_queue, &msg, BEKEN_WAIT_FOREVER) != kNoErr) {
            break;
        }
        s_pbap_event_queue_count--;
        if (msg.event == PBAP_PCE_DEMO_EXIT_EVT) {
            if (msg.data_ptr) {
                os_free(msg.data_ptr);
                msg.data_ptr = NULL;
            }
            while (rtos_pop_from_queue(&s_pbap_event_queue, &msg, BEKEN_NO_WAIT) == kNoErr) {
                if (msg.data_ptr) {
                    os_free(msg.data_ptr);
                }
                s_pbap_event_queue_count--;
            }
            rtos_deinit_queue(&s_pbap_event_queue);
            s_pbap_event_queue = (beken_queue_t)0;
            s_pbap_event_queue_count = 0;
            rtos_delete_thread(NULL);
            return;
        }
        pbap_pce_handle_event(&msg);
        if (msg.data_ptr) {
            os_free(msg.data_ptr);
            msg.data_ptr = NULL;
        }
    }
    rtos_delete_thread(NULL);
}

/** Callback from BT stack: only enqueue event and return (do not block stack). */
static void pbap_pce_demo_cb(bk_pbap_pce_cb_event_t event, bk_pbap_pce_cb_param_t *param)
{
    pbap_event_msg_t msg = {0};
    bk_err_t ret;

    if (!param) {
        return;
    }

    msg.event = event;

    switch (event) {
    case BK_PBAP_PCE_INIT_EVT:
        msg.status = param->init.status;
        break;
    case BK_PBAP_PCE_DEINIT_EVT:
        msg.status = param->uninit.status;
        break;
    case BK_PBAP_PCE_CONNECT_CFM_EVT:
        msg.status = param->connect_cfm.status;
        os_memcpy(msg.bd_addr, param->connect_cfm.bd_addr, BK_BD_ADDR_LEN);
        msg.remote_supported_repositories = param->connect_cfm.remote_supported_repositories;
        msg.remote_supported_features      = param->connect_cfm.remote_supported_features;
        break;
    case BK_PBAP_PCE_DISCONNECT_CFM_EVT:
        msg.status = param->disconnect_cfm.status;
        os_memcpy(msg.bd_addr, param->disconnect_cfm.bd_addr, BK_BD_ADDR_LEN);
        break;
    case BK_PBAP_PCE_GET_PHONEBOOK_CFM_EVT:
        msg.status   = param->get_phonebook_cfm.status;
        os_memcpy(msg.bd_addr, param->get_phonebook_cfm.bd_addr, BK_BD_ADDR_LEN);
        msg.data_len = param->get_phonebook_cfm.data_len;
        msg.data_ptr = NULL;
        if (msg.data_len > 0 && param->get_phonebook_cfm.data &&
            msg.data_len <= BK_PBAP_PCE_DEFAULT_MAX_RECV_SIZE) {
            msg.data_ptr = (uint8_t *)os_malloc(msg.data_len);
            if (msg.data_ptr) {
                os_memcpy(msg.data_ptr, param->get_phonebook_cfm.data, msg.data_len);
            }
        }
        break;
    case BK_PBAP_PCE_GET_VCARD_LIST_CFM_EVT:
        msg.status   = param->get_vcard_list_cfm.status;
        os_memcpy(msg.bd_addr, param->get_vcard_list_cfm.bd_addr, BK_BD_ADDR_LEN);
        msg.data_len = param->get_vcard_list_cfm.data_len;
        msg.data_ptr = NULL;
        if (msg.data_len > 0 && param->get_vcard_list_cfm.data &&
            msg.data_len <= BK_PBAP_PCE_DEFAULT_MAX_RECV_SIZE) {
            msg.data_ptr = (uint8_t *)os_malloc(msg.data_len);
            if (msg.data_ptr) {
                os_memcpy(msg.data_ptr, param->get_vcard_list_cfm.data, msg.data_len);
            }
        }
        break;
    case BK_PBAP_PCE_GET_VCARD_CFM_EVT:
        msg.status   = param->get_vcard_cfm.status;
        os_memcpy(msg.bd_addr, param->get_vcard_cfm.bd_addr, BK_BD_ADDR_LEN);
        msg.data_len = param->get_vcard_cfm.data_len;
        msg.data_ptr = NULL;
        if (msg.data_len > 0 && param->get_vcard_cfm.data &&
            msg.data_len <= BK_PBAP_PCE_DEFAULT_MAX_RECV_SIZE) {
            msg.data_ptr = (uint8_t *)os_malloc(msg.data_len);
            if (msg.data_ptr) {
                os_memcpy(msg.data_ptr, param->get_vcard_cfm.data, msg.data_len);
            }
        }
        break;
    case BK_PBAP_PCE_SET_PHONEBOOK_CFM_EVT:
        msg.status = param->set_phonebook_cfm.status;
        os_memcpy(msg.bd_addr, param->set_phonebook_cfm.bd_addr, BK_BD_ADDR_LEN);
        break;
    case BK_PBAP_PCE_GET_SIZE_CFM_EVT:
        msg.status          = param->get_size_cfm.status;
        os_memcpy(msg.bd_addr, param->get_size_cfm.bd_addr, BK_BD_ADDR_LEN);
        msg.phonebook_size  = param->get_size_cfm.phonebook_size;
        break;
    case BK_PBAP_PCE_ABORT_CFM_EVT:
        msg.status = param->abort_cfm.status;
        os_memcpy(msg.bd_addr, param->abort_cfm.bd_addr, BK_BD_ADDR_LEN);
        break;
    default:
        break;
    }

    ret = rtos_push_to_queue(&s_pbap_event_queue, &msg, BEKEN_NO_WAIT);
    if (ret != kNoErr) {
        LOGE("PBAP event queue full, drop event %d\n", (int)event);
        if(msg.data_ptr)
        {
            os_free(msg.data_ptr);
            msg.data_ptr = NULL;
        }
    }else
    {
        s_pbap_event_queue_count++;
    }
}

int pbap_pce_demo_init(void)
{
    int ret;

    LOGI("%s\n", __func__);

    if(s_pbap_event_queue == NULL) 
    {
        ret = rtos_init_queue(&s_pbap_event_queue, "pbap_evt_q",
                            sizeof(pbap_event_msg_t), PBAP_EVENT_QUEUE_LEN);
        if (ret != kNoErr) {
            LOGE("%s: rtos_init_queue failed %d\n", __func__, ret);
            return -1;
        }
    }

    if(s_pbap_event_thread == NULL) 
    {
        ret = rtos_create_thread(&s_pbap_event_thread, 5, "pbap_evt",
                                (beken_thread_function_t)pbap_pce_event_task,
                                PBAP_EVENT_TASK_STACK, NULL);
        if (ret != kNoErr) {
            LOGE("%s: rtos_create_thread pbap_evt failed %d\n", __func__, ret);
            rtos_deinit_queue(&s_pbap_event_queue);
            return -1;
        }
    }

    ret = bk_bt_pbap_pce_register_callback(pbap_pce_demo_cb);
    if (ret != BK_OK) {
        LOGE("%s bk_bt_pbap_pce_register_callback err %d\n", __func__, ret);
        pbap_pce_demo_exit_event_task_and_clear();
        return -1;
    }

    ret = bk_bt_pbap_pce_init();
    if (ret != BK_OK) {
        LOGE("%s bk_bt_pbap_pce_init err %d\n", __func__, ret);
        bk_bt_pbap_pce_register_callback(NULL);
        pbap_pce_demo_exit_event_task_and_clear();
        return -1;
    }

    return 0;
}

void pbap_pce_demo_deinit(void)
{
    (void)bk_bt_pbap_pce_deinit();
    bk_bt_pbap_pce_register_callback(NULL);
    pbap_pce_demo_exit_event_task_and_clear();
}

void pbap_pce_demo_connect(const uint8_t *bd_addr, uint8_t auth_required, const char *pin, const char *user_id)
{
    bk_pbap_pce_connect_param_t param = {0};
    int ret;

    if (!bd_addr) {
        LOGE("%s: bd_addr NULL\n", __func__);
        return;
    }

    os_memcpy(param.bd_addr, bd_addr, BK_BD_ADDR_LEN);
    param.auth_required = auth_required ? 1 : 0;
    param.max_recv_size = BK_PBAP_PCE_DEFAULT_MAX_RECV_SIZE;

    if (auth_required) 
    {
        if (pin && pin[0]) 
        {
            param.pin = (const uint8_t *)pin;
            param.pin_len = (uint16_t)os_strlen(pin);
            if (param.pin_len > BK_PBAP_PCE_MAX_PIN_LEN) {
                param.pin_len = BK_PBAP_PCE_MAX_PIN_LEN;
            }
        }
        
        if (user_id && user_id[0]) {
            param.user_id = (const uint8_t *)user_id;
            param.user_id_len = (uint16_t)os_strlen(user_id);
            if (param.user_id_len > BK_PBAP_PCE_MAX_USER_ID_LEN) {
                param.user_id_len = BK_PBAP_PCE_MAX_USER_ID_LEN;
            }
        }
    }

    ret = bk_bt_pbap_pce_connect(&param);
    if (ret != BK_OK) {
        LOGE("%s bk_bt_pbap_pce_connect err %d\n", __func__, ret);
    }
}

void pbap_pce_demo_disconnect(const uint8_t *bd_addr)
{
    int ret;
    if (!bd_addr) {
        LOGE("%s: bd_addr NULL\n", __func__);
        return;
    }
    ret = bk_bt_pbap_pce_disconnect(bd_addr);
    if (ret != BK_OK) {
        LOGE("%s bk_bt_pbap_pce_disconnect err %d\n", __func__, ret);
    }
}

void pbap_pce_demo_send_auth_response(const uint8_t *bd_addr, const char *pin, const char *user_id)
{
    int ret;
    uint16_t pin_len = 0;
    uint16_t user_id_len = 0;

    if (!bd_addr) {
        LOGE("%s: bd_addr NULL\n", __func__);
        return;
    }

    if (pin && pin[0]) {
        pin_len = (uint16_t)os_strlen(pin);
        if (pin_len > BK_PBAP_PCE_MAX_PIN_LEN) {
            pin_len = BK_PBAP_PCE_MAX_PIN_LEN;
        }
    }

    if (user_id && user_id[0]) {
        user_id_len = (uint16_t)os_strlen(user_id);
        if (user_id_len > BK_PBAP_PCE_MAX_USER_ID_LEN) {
            user_id_len = BK_PBAP_PCE_MAX_USER_ID_LEN;
        }
    }

    ret = bk_bt_pbap_pce_send_auth_response(bd_addr,
                                         pin ? (const uint8_t *)pin : NULL,
                                         pin_len,
                                         user_id ? (const uint8_t *)user_id : NULL,
                                         user_id_len);
    if (ret != BK_OK) {
        LOGE("%s bk_bt_pbap_pce_send_auth_response err %d\n", __func__, ret);
    }
}

#define PBAP_OBJ_NAME_BUF_LEN 64
static char s_pbap_obj_name_buf[PBAP_OBJ_NAME_BUF_LEN];

static const char *get_object_name(const char *object, const char *default_val)
{
    if (object && object[0]) {
        strncpy(s_pbap_obj_name_buf, object, PBAP_OBJ_NAME_BUF_LEN - 1);
        s_pbap_obj_name_buf[PBAP_OBJ_NAME_BUF_LEN - 1] = '\0';
        return s_pbap_obj_name_buf;
    }
    return default_val;
}

void pbap_pce_demo_get_phonebook(const uint8_t *bd_addr, const char *object,
                                 uint16_t max_list_count, uint16_t list_start_offset)
{
    bk_pbap_pce_get_param_t get_param = {0};
    int ret;

    if (!bd_addr) {
        LOGE("%s: bd_addr NULL\n", __func__);
        return;
    }

    vcard_parse_name_and_tel_reset();
    get_param.get_phonebook_param.object_name = get_object_name(object, TELECOM_PHONEBOOK_OBJECT_NAME);
    get_param.get_phonebook_param.max_list_count = max_list_count;
    get_param.get_phonebook_param.list_start_offset = list_start_offset;
    get_param.get_phonebook_param.format = BK_PBAP_PCE_FORMAT_3_0;

    ret = bk_bt_pbap_pce_get_phonebook(bd_addr, &get_param);
    if (ret != BK_OK) {
        LOGE("%s bk_bt_pbap_pce_get_phonebook err %d\n", __func__, ret);
    }
}

void pbap_pce_demo_set_phonebook(const uint8_t *bd_addr, uint8_t path_type, const char *path_name)
{
    bk_pbap_pce_set_path_param_t set_param = {0};
    int ret;

    if (!bd_addr) {
        LOGE("%s: bd_addr NULL\n", __func__);
        return;
    }

    set_param.path_type = (bk_pbap_pce_set_path_type_t)path_type;
    set_param.path_name = path_name;

    ret = bk_bt_pbap_pce_set_phonebook(bd_addr, &set_param);
    if (ret != BK_OK) {
        LOGE("%s bk_bt_pbap_pce_set_phonebook err %d\n", __func__, ret);
    }
}

void pbap_pce_demo_get_vcard_list(const uint8_t *bd_addr, const char *object,
                                  uint16_t max_list_count, uint16_t list_start_offset)
{
    bk_pbap_pce_get_param_t get_param = {0};
    int ret;

    if (!bd_addr) {
        LOGE("%s: bd_addr NULL\n", __func__);
        return;
    }

    vcard_list_parse_cards_reset();

    get_param.get_vcard_list_param.object_name = get_object_name(object, "pb");
    get_param.get_vcard_list_param.max_list_count = max_list_count;
    get_param.get_vcard_list_param.list_start_offset = list_start_offset;

    ret = bk_bt_pbap_pce_get_vcard_list(bd_addr, &get_param);
    if (ret != BK_OK) {
        LOGE("%s bk_bt_pbap_pce_get_vcard_list err %d\n", __func__, ret);
    }
}

void pbap_pce_demo_get_vcard(const uint8_t *bd_addr, const char *object)
{
    bk_pbap_pce_get_param_t get_param = {0};
    int ret;

    if (!bd_addr) {
        LOGE("%s: bd_addr NULL\n", __func__);
        return;
    }

    vcard_parse_name_and_tel_reset();
    get_param.get_vcard_param.object_name = get_object_name(object, "0.vcf");

    ret = bk_bt_pbap_pce_get_vcard(bd_addr, &get_param);
    if (ret != BK_OK) {
        LOGE("%s bk_bt_pbap_pce_get_vcard err %d\n", __func__, ret);
    }
}

void pbap_pce_demo_get_size(const uint8_t *bd_addr, const char *object)
{
    bk_pbap_pce_get_param_t get_param = {0};
    bk_pbap_pce_status_t status;

    if (!bd_addr) {
        LOGE("%s: bd_addr NULL\n", __func__);
        return;
    }

    get_param.get_size_param.object_name = get_object_name(object, TELECOM_PHONEBOOK_OBJECT_NAME);

    status = bk_bt_pbap_pce_get_size(bd_addr, &get_param);
    if (status != BK_PBAP_PCE_SUCCESS && status != BK_PBAP_PCE_CONTINUE) {
        LOGE("%s bk_bt_pbap_pce_get_size returns %s\n", __func__, pbap_status_str(status));
    }
}

void pbap_pce_demo_abort(const uint8_t *bd_addr)
{
    int ret;
    if (!bd_addr) {
        LOGE("%s: bd_addr NULL\n", __func__);
        return;
    }
    ret = bk_bt_pbap_pce_abort(bd_addr);
    if (ret != BK_OK) {
        LOGE("%s bk_bt_pbap_pce_abort err %d\n", __func__, ret);
    }
}
