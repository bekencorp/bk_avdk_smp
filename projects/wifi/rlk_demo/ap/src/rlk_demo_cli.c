#include "rlk_common.h"


#define RLK_CMD_CNT (sizeof(s_rlk_commands) / sizeof(struct cli_command))

static int hex2num_rlkcli(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static int hex2byte_rlkcli(const char *hex)
{
    int a, b;
    a = hex2num_rlkcli(*hex++);
    if (a < 0)
        return -1;
    b = hex2num_rlkcli(*hex++);
    if (b < 0)
        return -1;
    return (a << 4) | b;
}

int hexstr2bin_rlkcli(const char *hex, u8 *buf, size_t len)
{
    size_t i;
    int a;
    const char *ipos = hex;
    u8 *opos = buf;

    for (i = 0; i < len; i++) {
        a = hex2byte_rlkcli(ipos);
        if (a < 0)
            return -1;
        *opos++ = a;
        ipos += 2;
    }
    return 0;
}

void cli_rlk_iperf_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint8_t id;
    //sscanf bug: cant detect uint8_t size point
    uint32_t mac_addr_temp[6];
    int is_server_mode = 0;
    int is_client_mode = 0;
    int mode = 0;
    uint32_t value;

    if (rlk_param_find(argc, argv, "-h") || (argc == 1))
    {
        goto __usage;
    }
    else if (rlk_param_find(argc, argv, "-stop") || rlk_param_find(argc, argv, "--stop"))
    {
        rlk_iperf_stop();
        return;
    }

    is_server_mode = rlk_param_find(argc, argv, "-s");
    is_client_mode = rlk_param_find(argc, argv, "-c");

    if (((is_client_mode) && (is_server_mode))
        || ((is_server_mode) && (is_client_mode)))
    {
        goto __usage;
    }

    if (is_client_mode)
    {
        mode = RLK_IPERF_MODE_CLIENT;
    }
    else
    {
        mode = RLK_IPERF_MODE_SERVER;
    }

    if (is_client_mode)
    {
        id = rlk_param_find_id(argc, argv, "-c");
        if (RLK_INVALID_INDEX != id)
        {
            memcpy(rlk_iperf_env.mac_str,argv[1],RLK_MAC_STRING_LEN);
            //input 3 section
            if(strlen(argv[id + 1]) < 9)
            {
                rlk_iperf_env.mac_addr[0] = 0xc8;
                rlk_iperf_env.mac_addr[1] = 0x47;
                rlk_iperf_env.mac_addr[2] = 0x8c;
                sscanf(argv[id + 1], "%x:%x:%x",&mac_addr_temp[3], &mac_addr_temp[4], &mac_addr_temp[5]);

                rlk_iperf_env.mac_addr[3] = mac_addr_temp[3];
                rlk_iperf_env.mac_addr[4] = mac_addr_temp[4];
                rlk_iperf_env.mac_addr[5] = mac_addr_temp[5];
            }//input 6 section
            else
            {
                sscanf(argv[id + 1], "%x:%x:%x:%x:%x:%x",
                &mac_addr_temp[0], &mac_addr_temp[1], &mac_addr_temp[2],
                &mac_addr_temp[3], &mac_addr_temp[4], &mac_addr_temp[5]);

                for(int i=0;i<6;i++)
                {
                    rlk_iperf_env.mac_addr[i] = (uint8_t)(mac_addr_temp[i] & 0xff);
                }
            }

            if (argc - 1 < id + 1)
            {
                goto __usage;
            }
        }
    }

    if (is_client_mode)
    {
        id = rlk_param_find_id(argc, argv, "-n");

        if (RLK_INVALID_INDEX != id)
        {
            rlk_iperf_env.size = atoi(argv[id + 1]);
            if (argc - 1 < id + 1)
            {
                goto __usage;
            }
        }
    }

    id = rlk_param_find_id(argc, argv, "-i");

    if (RLK_INVALID_INDEX != id)
    {
        rlk_iperf_env.interval = atoi(argv[id + 1]);
        if (argc - 1 < id + 1)
        {
            goto __usage;
        }
    }

    if (is_client_mode)
    {
        id = rlk_param_find_id(argc, argv, "-t");
        if (RLK_INVALID_INDEX != id)
        {
            rlk_iperf_env.s_time = atoi(argv[id + 1]);
            if(rlk_iperf_env.s_time == 0)
            {
                rlk_iperf_env.s_time = RLK_IPERF_DEFAULT_TIME;
            }
            if (argc - 1 < id + 1)
            {
                goto __usage;
            }
        }
    }

    id = rlk_param_find_id(argc, argv, "-b");
    if (RLK_INVALID_INDEX != id) {
        if (argv[id + 1] == NULL) {
            rlk_iperf_env.speed_limit = 0;
        }
        else {
            rlk_iperf_env.speed_limit = atoi(argv[id + 1]);

            if ((rlk_iperf_env.speed_limit == 0) || argc - 1 < id + 1)
                goto __usage;

            value = strlen(argv[id + 1]);
            if (value > 1) {
                if (argv[id + 1][value - 1] == 'k') {
                    rlk_iperf_env.speed_limit *= 1000;
                } else if (argv[id + 1][value - 1] == 'K') {
                    rlk_iperf_env.speed_limit *= 1024;
                } else if (argv[id + 1][value - 1] == 'm') {
                    rlk_iperf_env.speed_limit *= 1000 * 1000;
                } else if (argv[id + 1][value - 1] == 'M') {
                    rlk_iperf_env.speed_limit *= 1024 * 1024;
                } else {
                    goto __usage;
                }
            }
        }
    }

    RLK_LOGD("before mode %d size %d interval %d s_time %d\r\n",rlk_iperf_env.mode,rlk_iperf_env.size,rlk_iperf_env.interval,rlk_iperf_env.s_time);
    rlk_iperf_defaults_set();
    RLK_LOGD("after mode %d size %d interval %d s_time %d\r\n",rlk_iperf_env.mode,rlk_iperf_env.size,rlk_iperf_env.interval,rlk_iperf_env.s_time);

    if (is_client_mode)
    {
        RLK_LOGD("rlk_iperf_env.mac_addr = ");
        os_printf(" %x:%x:%x:%x:%x:%x\r\n",rlk_iperf_env.mac_addr[0],rlk_iperf_env.mac_addr[1],rlk_iperf_env.mac_addr[2],rlk_iperf_env.mac_addr[3],rlk_iperf_env.mac_addr[4],rlk_iperf_env.mac_addr[5]);
        #if 0
        for(int i = 0; i < 6;i++)
        {
            RLK_LOG_RAW("0x%x ",rlk_iperf_env.mac_addr[i]);
        }
        RLK_LOGD("\n");
        #endif
        RLK_LOGD("rlk_iperf_env.mac_addr_str = %s\n",rlk_iperf_env.mac_str);
    }

    rlk_iperf_start(mode);

    return;

__usage:
    rlk_iperf_usage();
}

void cli_rlk_ping_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint8_t id;
    //sscanf bug: cant detect uint8_t size point
    uint32_t mac_addr_temp[6];
    if (rlk_param_find(argc, argv, "-h") || (argc == 1))
        goto __usage;
    else if (rlk_param_find(argc, argv, "-stop") || rlk_param_find(argc, argv, "--stop"))
    {
        rlk_ping_stop();
        return;
    }
    memcpy(rlk_ping_env.mac_str,argv[1],RLK_MAC_STRING_LEN);
    //input 3 section
    if(strlen(argv[1]) < 9)
    {
        rlk_ping_env.mac_addr[0] = 0xc8;
        rlk_ping_env.mac_addr[1] = 0x47;
        rlk_ping_env.mac_addr[2] = 0x8c;
        sscanf(argv[1], "%x:%x:%x",&mac_addr_temp[3], &mac_addr_temp[4], &mac_addr_temp[5]);
        
        rlk_ping_env.mac_addr[3] = mac_addr_temp[3];
        rlk_ping_env.mac_addr[4] = mac_addr_temp[4];
        rlk_ping_env.mac_addr[5] = mac_addr_temp[5];
    }//input 6 section
    else
    {
        sscanf(argv[1], "%x:%x:%x:%x:%x:%x",
        &mac_addr_temp[0], &mac_addr_temp[1], &mac_addr_temp[2],
        &mac_addr_temp[3], &mac_addr_temp[4], &mac_addr_temp[5]);

        for(int i=0;i<6;i++)
        {
            rlk_ping_env.mac_addr[i] = (uint8_t)(mac_addr_temp[i] & 0xff);
        }
    }
    id = rlk_param_find_id(argc, argv, "-c");
    if (RLK_INVALID_INDEX != id)
    {
        rlk_ping_env.count = atoi(argv[id + 1]);
        if (argc - 1 < id + 1)
            goto __usage;
    }

    id = rlk_param_find_id(argc, argv, "-i");
    if (RLK_INVALID_INDEX != id)
    {
        rlk_ping_env.interval = atoi(argv[id + 1]);
        if (argc - 1 < id + 1)
            goto __usage;
    }

    id = rlk_param_find_id(argc, argv, "-s");
    if (RLK_INVALID_INDEX != id)
    {
        rlk_ping_env.size = atoi(argv[id + 1]);
        if (argc - 1 < id + 1)
            goto __usage;
    }

    id = rlk_param_find_id(argc, argv, "-t");
    if (RLK_INVALID_INDEX != id)
    {
        rlk_ping_env.timeout = atoi(argv[id + 1]);
        if (argc - 1 < id + 1)
            goto __usage;
    }

    rlk_ping_default_set();

    RLK_LOGD("c=%d,s=%d,i=%d,t=%d\n",rlk_ping_env.count,rlk_ping_env.size,rlk_ping_env.interval,rlk_ping_env.timeout);
    RLK_LOGD("rlk_ping_env.mac_addr = ");
    for(int i =0; i<6;i++)
    {
        BK_LOG_RAW("0x%x ",rlk_ping_env.mac_addr[i]);
    }
    RLK_LOGD("\n");
    RLK_LOGD("rlk_ping_env.mac_addr_str = %s\n",rlk_ping_env.mac_str);

    rlk_ping_start();

    return;

__usage:
    rlk_ping_usage();
}

bk_err_t bk_rlk_acs_cfm_callback(const uint32_t chanstatus[],uint32_t num_channel,uint32_t best_channel)
{
    if (num_channel <= 0)
        return BK_FAIL;

    RLK_LOGD("Best chan(decrease) : \r\n");
    for (int i = 0; i < num_channel; i++)
    {
        BK_LOG_RAW("[%d] %d ", i,chanstatus[i]);
    }
    BK_LOG_RAW("\r\n ");

    RLK_LOGD("Best channel is %d\n",best_channel);

    return BK_OK;
}

void cli_rlk_acs_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t switch_auto = 0;

    if (argc == 1)
    {
        BK_LOG_ON_ERR(bk_rlk_register_acs_cfm_cb(bk_rlk_acs_cfm_callback));
        bk_rlk_acs_check();
    }
    else
    {
        if (os_strcmp(argv[1], "auto_switch") == 0)
        {
            switch_auto = os_strtoul(argv[2], NULL, 10);
            bk_rlk_set_acs_auto_switch_chan(switch_auto);
            //bk_rlk_acs_check();
        }
    }


}

bk_err_t bk_rlk_scan_cfm_callback(bk_rlk_scan_result_t *result)
{
    if (!result || !result->masters)
        return BK_FAIL;

    RLK_LOGD("BK-RLK scan %d masters: \rs\n", result->master_num);
    for(int i = 0; i < result->master_num; i++ )
    {
        BK_LOG_RAW("%d: %s, ", i + 1, result->masters->ssid);
        BK_LOG_RAW("%x:%x:%x:%x:%x:%x, ",result->masters->bssid[0],result->masters->bssid[1],result->masters->bssid[2],
                                         result->masters->bssid[3],result->masters->bssid[4],result->masters->bssid[5]);
        BK_LOG_RAW("Channal:%d, ", result->masters->channel);
        BK_LOG_RAW("RSSI=%d \r\n", result->masters->rssi);
    }

    return BK_OK;
}

void cli_rlk_scan_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    int ret = 0;
    bk_rlk_scan_info_t info = {0};

    if (argc < 2)
    {
        BK_LOG_ON_ERR(bk_rlk_register_scan_cfm_cb(bk_rlk_scan_cfm_callback));
        info.scan_mode = BK_RLK_ALL_SCAN;
        bk_rlk_scan(&info);
    }
    else
    {
        info.scan_mode = os_strtoul(argv[1], NULL, 10);

        if (info.scan_mode == BK_RLK_ONE_SSID_SCAN)
        {
            info.ssid = argv[2];

            if (!info.ssid)
            {
                RLK_LOGE("ssid param is invalid\r\n");
                return;
            }

            BK_LOG_ON_ERR(bk_rlk_register_scan_cfm_cb(bk_rlk_scan_cfm_callback));
            bk_rlk_scan(&info);
        }
        else if (info.scan_mode == BK_RLK_ONE_BSSID_SCAN)
        {
            info.ssid = argv[2];
            //Attention send bssid is c8478cf907cc
            ret = hexstr2bin_rlkcli(argv[2], info.bssid, BK_RLK_BSSID_LEN);
            if (ret < 0)
            {
                RLK_LOGE("bssid param is invalid\r\n");
                return;
            }

            BK_LOG_ON_ERR(bk_rlk_register_scan_cfm_cb(bk_rlk_scan_cfm_callback));
            bk_rlk_scan(&info);

        }
        else
        {
            RLK_LOGE("BK-RLK scan mode error %d \rs\n",info.scan_mode);
        }
    }
}

void cli_rlk_dubid_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_rlk_extra_ies_info_t ies_info;

    ies_info.extra_ies_len = 3;
    ies_info.extra_ies = os_malloc(ies_info.extra_ies_len);
    ies_info.extra_ies[0] = 0xc8;
    ies_info.extra_ies[1] = 0x47;
    ies_info.extra_ies[2] = 0x8c;

    bk_rlk_set_role(BK_RLK_DUBID,&ies_info);

    if (ies_info.extra_ies)
        os_free(ies_info.extra_ies);
}

void cli_rlk_master_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
   bk_rlk_set_role(BK_RLK_MASTER,NULL);
}

void cli_rlk_slave_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *tp_ssid = NULL;
    uint8_t tp_bssid[BK_RLK_BSSID_LEN] = {0};
    int ret = 0;

    if (argc < 2) {
        RLK_LOGE("CMD param is invalid\r\n");
        return;
    }

    if (os_strcmp(argv[1], "role") == 0) {
        bk_rlk_set_role(BK_RLK_SLAVE,NULL);
    }
    else if (os_strcmp(argv[1], "ssid") == 0) {

        bk_rlk_set_role(BK_RLK_SLAVE,NULL);

        tp_ssid = argv[2];

        if (!tp_ssid)
        {
            RLK_LOGE("ssid param is invalid\r\n");
            return;
        }

        bk_rlk_slave_app_init((char *)tp_ssid);
    }
    else if (os_strcmp(argv[1], "bssid") == 0) {

        bk_rlk_set_role(BK_RLK_SLAVE,NULL);

        ret = hexstr2bin_rlkcli(argv[2], tp_bssid, BK_RLK_BSSID_LEN);
        if (ret < 0)
        {
            RLK_LOGE("bssid param is invalid\r\n");
            return;
        }

        bk_rlk_slave_bssid_app_init(tp_bssid);
    }
    else
    {
        RLK_LOGE("CMD param is invalid\r\n");
    }

}

void cli_rlk_set_chan_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint8_t chan ;
    if (argc < 2) {
        return;
    }

    chan = (uint8_t)os_strtoul(argv[1], NULL, 10);
    BK_LOG_ON_ERR(bk_rlk_set_channel(chan));
    RLK_LOGD("Set chan %d \r\n",bk_rlk_get_channel());
}

static const struct cli_command s_rlk_commands[] = {
    {"rlk_ping", "rlk_ping <mac>", cli_rlk_ping_cmd},
    {"rlk_acs", "rlk_acs <auto_switch>", cli_rlk_acs_cmd},
    {"rlk_scan", "rlk_scan", cli_rlk_scan_cmd},
    {"rlk_master", "rlk_master", cli_rlk_master_cmd},
    {"rlk_slave", "rlk_slave [role][ssid][bssid]", cli_rlk_slave_cmd},
    {"rlk_dubid", "rlk_dubid", cli_rlk_dubid_cmd},
    {"rlk_chan", "rlk_chan", cli_rlk_set_chan_cmd},
    {"rlk_iperf", "rlk_iperf [-h]", cli_rlk_iperf_cmd},
};

int cli_rlk_init(void)
{
    return cli_register_commands(s_rlk_commands, RLK_CMD_CNT);
}

