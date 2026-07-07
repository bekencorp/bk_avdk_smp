/**
 * @file pbap_pce_demo_cli.c
 *
 * PBAP PCE CLI commands for bk_dm_pbap_pce.h API functional testing.
 */

#include "cli.h"
#include "os/os.h"
#include "pbap_pce_demo_cli.h"
#include <driver/uart.h>
/* Connected device address: set on connect, used by all commands that need an address. */
static uint8_t s_current_mac[6];
static uint8_t s_current_mac_valid;

#define REQUIRE_CONNECTED() do { \
    if (!s_current_mac_valid) { \
        CLI_LOGE("pbap: not connected. connect first.\n"); \
        goto __pbap_error; \
    } \
    os_memcpy(mac_final, s_current_mac, 6); \
} while (0)

void pbap_pce_demo_cli_set_connection_state(const uint8_t *bd_addr, uint8_t connected)
{
    if (connected && bd_addr)
    {
        os_memcpy(s_current_mac, bd_addr, sizeof(s_current_mac));
        s_current_mac_valid = 1;
    }
    else
    {
        s_current_mac_valid = 0;
        os_memset(s_current_mac, 0, sizeof(s_current_mac));
    }
}

/*
 * cmd example (after pbap init):
 *
 *   pbap init
 *   pbap deinit
 *   pbap connect xx:xx:xx:xx:xx:xx
 *   pbap conn xx:xx:xx:xx:xx:xx auth 123456
 *   pbap auth 123456
 *   pbap disconnect
 *
 *   pbap get_phonebook pb 0
 *   pbap get_phonebook mch 0
 * 
 *   pbap set_path root
 *   pbap set_path child telecom
 *   pbap set_path child pb
 *   pbap set_path child cch
 * 
 *   pbap get_vcard_list pb 0
 *   pbap get_vcard_list cch 0
 * 
 *   pbap get_vcard 0.vcf
 */

static void pbap_usage(void)
{
    CLI_LOGI("PBAP PCE Usage (uses connected device address):\n"
             "  pbap init\n"
             "  pbap deinit\n"
             "  pbap connect xx:xx:xx:xx:xx:xx [auth [pin [user_id]]]\n"
             "  pbap disconnect\n"
             "  pbap auth <pin> [user_id]\n"
             "  pbap get_phonebook pb|mch|och|ich|cch|spd|fav [offset]\n"
             "  pbap set_path root|parent|child [folder_name]\n"
             "  pbap get_vcard_list [folder_name] [offset]\n"
             "  pbap get_vcard [vcard_name]\n"
             "  pbap get_size pb|mch|och|ich|cch|spd|fav\n"
             "  pbap abort\n");
}

/* Map object shorthand to PBAP path. Returns path string or NULL if unknown. */
static const char *get_phonebook_object_name(const char *obj)
{
    if (!obj)
    {
        return NULL;
    }
    if (os_strcmp(obj, "pb") == 0)  return TELECOM_PHONEBOOK_OBJECT_NAME;
    if (os_strcmp(obj, "mch") == 0) return TELECOM_MISSED_CALLED_HISTORY_OBJECT_NAME;
    if (os_strcmp(obj, "och") == 0) return TELECOM_OUTGOING_CALLED_HISTORY_OBJECT_NAME;
    if (os_strcmp(obj, "ich") == 0) return TELECOM_INCOMING_CALLED_HISTORY_OBJECT_NAME;
    if (os_strcmp(obj, "cch") == 0) return TELECOM_COMBINED_CALLED_HISTORY_OBJECT_NAME;
    if (os_strcmp(obj, "spd") == 0) return TELECOM_SPEED_DIAL_OBJECT_NAME;
    if (os_strcmp(obj, "fav") == 0) return TELECOM_FAVORITES_CONTACTS_OBJECT_NAME;
    CLI_LOGI("pbap object: %s\n", obj);
    return obj;
}

/* Parse "xx:xx:xx:xx:xx:xx" string without sscanf. Returns 0 on success, -1 on error. */
static int parse_mac(const char *str, uint8_t *mac_final)
{
    char hex[3] = {0};
    if (!str || !mac_final) {
        return -1;
    }
    for (int i = 0; i < 6; i++) {
        int off = i * 3;
        if (str[off] == '\0' || str[off + 1] == '\0') {
            return -1;
        }
        hex[0] = str[off];
        hex[1] = str[off + 1];
        hex[2] = '\0';
        unsigned long val = os_strtoul(hex, NULL, 16);
        if (val > 0xFF) {
            return -1;
        }
        mac_final[i] = (uint8_t)val;
        if (i < 5 && str[off + 2] != ':') {
            return -1;
        }
    }
    return 0;
}

static void cmd_pbap_demo(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = NULL;
    uint8_t mac_final[6] = {0};

    if (!argv || argc < 2 || !argv[1] || os_strcmp(argv[1], "-h") == 0) {
        pbap_usage();
        return;
    }

    if (os_strcmp(argv[1], "init") == 0) {
        if (pbap_pce_demo_init() != 0) {
            goto __pbap_error;
        }
    }
    else if (os_strcmp(argv[1], "deinit") == 0) {
        pbap_pce_demo_deinit();
    }
    else if (os_strcmp(argv[1], "connect") == 0) {
        if (argc < 3) {
            goto __pbap_usage;
        }
        if (parse_mac(argv[2], mac_final) != 0) {
            CLI_LOGE("pbap: invalid MAC\n");
            goto __pbap_error;
        }
        pbap_pce_demo_cli_set_connection_state(NULL, 0);
        uint8_t auth = 0;
        const char *pin = NULL;
        const char *user_id = NULL;
        if (argc >= 4 && os_strcmp(argv[3], "auth") == 0) {
            auth = 1;
            if (argc >= 5) {
                pin = argv[4];
            }
            if (argc >= 6) {
                user_id = argv[5];
            }
        }
        pbap_pce_demo_connect(mac_final, auth, pin, user_id);
    }
    else if (os_strcmp(argv[1], "disconnect") == 0) {
        REQUIRE_CONNECTED();
        pbap_pce_demo_disconnect(mac_final);
        s_current_mac_valid = 0;
    }
    else if (os_strcmp(argv[1], "auth") == 0) {
        REQUIRE_CONNECTED();
        if (argc < 3) {
            goto __pbap_usage;
        }
        const char *pin = argv[2];
        const char *user_id = (argc >= 4) ? argv[3] : NULL;
        pbap_pce_demo_send_auth_response(mac_final, pin, user_id);
    }
    else if (os_strcmp(argv[1], "get_phonebook") == 0) {
        REQUIRE_CONNECTED();
        if (argc < 3) {
            goto __pbap_usage;
        }
        const char *path = get_phonebook_object_name(argv[2]);
        if (path == NULL) {
            CLI_LOGE("pbap: obj must be pb|mch|och|ich|cch|spd|fav\n");
            goto __pbap_error;
        }
        uint16_t offset = (argc >= 4) ? (uint16_t)os_strtoul(argv[3], NULL, 10) : 0;
        pbap_pce_demo_get_phonebook(mac_final, path, 0, offset);
    }
    else if (os_strcmp(argv[1], "set_path") == 0) {
        REQUIRE_CONNECTED();
        if (argc < 3) {
            goto __pbap_usage;
        }
        uint8_t path_type = 0;
        const char *path_name = NULL;
        if (os_strcmp(argv[2], "root") == 0) {
            path_type = 0; /* BK_PBAP_PCE_SET_PATH_ROOT */
        } else if (os_strcmp(argv[2], "parent") == 0) {
            path_type = 1; /* BK_PBAP_PCE_SET_PATH_PARENT */
        } else if (os_strcmp(argv[2], "child") == 0) {
            path_type = 2; /* BK_PBAP_PCE_SET_PATH_CHILD */
            if (argc >= 4) {
                path_name = argv[3];
            }
        } else {
            CLI_LOGE("pbap: path must be root|parent|child\n");
            goto __pbap_error;
        }
        pbap_pce_demo_set_phonebook(mac_final, path_type, path_name);
    }
    else if (os_strcmp(argv[1], "get_vcard_list") == 0) {
        REQUIRE_CONNECTED();
        const char *obj = (argc >= 3) ? argv[2] : NULL;
        uint16_t offset = (argc >= 4) ? (uint16_t)os_strtoul(argv[3], NULL, 10) : 0;
        pbap_pce_demo_get_vcard_list(mac_final, obj, 0, offset);
    }
    else if (os_strcmp(argv[1], "get_vcard") == 0) {
        REQUIRE_CONNECTED();
        const char *obj = (argc >= 3) ? argv[2] : NULL;
        pbap_pce_demo_get_vcard(mac_final, obj);
    }
    else if (os_strcmp(argv[1], "get_size") == 0) {
        REQUIRE_CONNECTED();
        if (argc < 3) {
            goto __pbap_usage;
        }
        const char *path = get_phonebook_object_name(argv[2]);
        if (path == NULL) {
            CLI_LOGE("pbap: obj must be pb|mch|och|ich|cch|spd|fav\n");
            goto __pbap_error;
        }
        pbap_pce_demo_get_size(mac_final, path);
    }
    else if (os_strcmp(argv[1], "abort") == 0) {
        REQUIRE_CONNECTED();
        pbap_pce_demo_abort(mac_final);
    }
    else {
        goto __pbap_usage;
    }

    msg = CLI_CMD_RSP_SUCCEED;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    return;

__pbap_usage:
    pbap_usage();
    return;

__pbap_error:
    msg = CLI_CMD_RSP_ERROR;
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

static struct cli_command s_pbap_commands[] = {
    { "pbap", "PBAP PCE demo, see pbap -h", cmd_pbap_demo },
};

int cli_pbap_pce_demo_init(void)
{
    return cli_register_commands(s_pbap_commands, sizeof(s_pbap_commands) / sizeof(s_pbap_commands[0]));
}