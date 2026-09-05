#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "bluetooth_storage.h"

#if CONFIG_EASY_FLASH
    #include "easyflash.h"
#endif

#include "driver/uart.h"
#include <components/log.h>

#define TAG "bt_storage"

enum
{
    BT_STORAGE_DEBUG_LEVEL_ERROR,
    BT_STORAGE_DEBUG_LEVEL_WARNING,
    BT_STORAGE_DEBUG_LEVEL_INFO,
    BT_STORAGE_DEBUG_LEVEL_DEBUG,
    BT_STORAGE_DEBUG_LEVEL_VERBOSE,
};

#define BT_STORAGE_DEBUG_LEVEL BT_STORAGE_DEBUG_LEVEL_INFO

#define LOGE(format, ...) do{if(BT_STORAGE_DEBUG_LEVEL >= BT_STORAGE_DEBUG_LEVEL_ERROR)   BK_LOGE(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGW(format, ...) do{if(BT_STORAGE_DEBUG_LEVEL >= BT_STORAGE_DEBUG_LEVEL_WARNING) BK_LOGW(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGI(format, ...) do{if(BT_STORAGE_DEBUG_LEVEL >= BT_STORAGE_DEBUG_LEVEL_INFO)    BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGD(format, ...) do{if(BT_STORAGE_DEBUG_LEVEL >= BT_STORAGE_DEBUG_LEVEL_DEBUG)   BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGV(format, ...) do{if(BT_STORAGE_DEBUG_LEVEL >= BT_STORAGE_DEBUG_LEVEL_VERBOSE) BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)

static bt_user_storage_t *s_bt_user_storage;
static const uint8_t s_bt_empty_addr[6] = {0};
static const uint8_t s_bt_invaild_addr[6] = {0xff};

static uint8_t bluetooth_storage_is_addr_valid(uint8_t *addr);

static int32_t bluetooth_storage_alloc_linkkey_info(uint8_t *addr)
{
    int32_t ret = 0;

    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

    ret = bluetooth_storage_find_linkkey_info_index(addr, NULL);

    if (ret >= 0)
    {
        return ret;
    }

    for (int i = 0; i < sizeof(s_bt_user_storage->dev) / sizeof(s_bt_user_storage->dev[0]); ++i)
    {
        if (!memcmp(s_bt_empty_addr, s_bt_user_storage->dev[i].addr, sizeof(s_bt_user_storage->dev[i].addr)) ||
                !memcmp(s_bt_invaild_addr, s_bt_user_storage->dev[i].addr, sizeof(s_bt_user_storage->dev[i].addr)))
        {
            memcpy(s_bt_user_storage->dev[i].addr, addr, sizeof(s_bt_user_storage->dev[i].addr));
            memset(s_bt_user_storage->dev[i].link_key, 0, sizeof(s_bt_user_storage->dev[i].link_key));

            return i;
        }
    }

    return -1;
}

static int32_t bluetooth_storage_free_linkkey_info_by_index(uint8_t index)
{
    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

    if (index >= 0 && index < sizeof(s_bt_user_storage->dev) / sizeof(s_bt_user_storage->dev[0]))
    {
        memset(&s_bt_user_storage->dev[index], 0, sizeof(s_bt_user_storage->dev[0]));
        return 0;
    }
    else
    {
        return -1;
    }
}

int32_t bluetooth_storage_find_linkkey_info_index(uint8_t *addr, uint8_t *key)
{
    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

    for (int i = 0; i < sizeof(s_bt_user_storage->dev) / sizeof(s_bt_user_storage->dev[0]); ++i)
    {
        if (!memcmp(addr, s_bt_user_storage->dev[i].addr, sizeof(s_bt_user_storage->dev[i].addr)))
        {
            if (key)
            {
                memcpy(key, s_bt_user_storage->dev[i].link_key, sizeof(s_bt_user_storage->dev[i].link_key));
            }

            return i;
        }
    }

    return -1;
}


int32_t bluetooth_storage_save_linkkey_info(uint8_t *addr, uint8_t *key)
{
    bt_user_storage_elem_t tmp_key;
    int32_t index = 0;
    int32_t ret = 0;
    int i = 0;

    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

    index = bluetooth_storage_find_linkkey_info_index(addr, NULL);

    memset(&tmp_key, 0, sizeof(tmp_key));

    if (index >= 0)
    {

    }
    else
    {
        index = bluetooth_storage_alloc_linkkey_info(addr);

        if (index >= 0)
        {

        }
        else
        {
            LOGW("overwrite 0 linkkey info, %02X:%02X:%02X:%02X:%02X:%02X",
                 s_bt_user_storage->dev[0].addr[5],
                 s_bt_user_storage->dev[0].addr[4],
                 s_bt_user_storage->dev[0].addr[3],
                 s_bt_user_storage->dev[0].addr[2],
                 s_bt_user_storage->dev[0].addr[1],
                 s_bt_user_storage->dev[0].addr[0]);
            index = 0;
        }
    }

    memcpy(tmp_key.addr, addr, sizeof(tmp_key.addr));
    memcpy(tmp_key.link_key, key, sizeof(tmp_key.link_key));

    for (i = index + 1; i < sizeof(s_bt_user_storage->dev) / sizeof(s_bt_user_storage->dev[0]); ++i)
    {
        memcpy(&s_bt_user_storage->dev[i - 1], &s_bt_user_storage->dev[i], sizeof(s_bt_user_storage->dev[0]));
    }

    memcpy(&s_bt_user_storage->dev[i - 1], &tmp_key, sizeof(tmp_key));
#if 0//CONFIG_EASY_FLASH_V4
    ret = ef_set_env_blob(BT_STORAGE_KEY, s_bt_user_storage, sizeof(*s_bt_user_storage));

    if (ret)
    {
        LOGE("ef_set_env_blob err %d", ret);
    }

#endif
    (void)ret;
    return i - 1;
}


int32_t bluetooth_storage_update_to_newest(uint8_t *addr)
{
    bt_user_storage_elem_t tmp_key;
    int i = 0;
    int32_t ret = 0;
    int32_t index = 0;

    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

    index = bluetooth_storage_find_linkkey_info_index(addr, NULL);

    if (index < 0)
    {
        return -1;
    }

    if (index >= sizeof(s_bt_user_storage->dev) / sizeof(s_bt_user_storage->dev[0]))
    {
        LOGE("index err %d", index);
        return -1;
    }

    memcpy(&tmp_key, &s_bt_user_storage->dev[index], sizeof(s_bt_user_storage->dev[index]));

    for (i = index + 1; i < sizeof(s_bt_user_storage->dev) / sizeof(s_bt_user_storage->dev[0]); ++i)
    {
        memcpy(&s_bt_user_storage->dev[i - 1], &s_bt_user_storage->dev[i], sizeof(s_bt_user_storage->dev[0]));
    }

    memcpy(&s_bt_user_storage->dev[i - 1], &tmp_key, sizeof(tmp_key));
#if 0//CONFIG_EASY_FLASH_V4
    ret = ef_set_env_blob(BT_STORAGE_KEY, s_bt_user_storage, sizeof(*s_bt_user_storage));

    if (ret)
    {
        LOGE("ef_set_env_blob err %d", ret);
    }

#endif
    (void)ret;
    return 0;
}

int32_t bluetooth_storage_get_newest_linkkey_info(uint8_t *addr, uint8_t *key)
{
    int i = 0;

    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

    for (i = sizeof(s_bt_user_storage->dev) / sizeof(s_bt_user_storage->dev[0]) - 1; i >= 0; --i)
    {
        if (memcmp(s_bt_empty_addr, s_bt_user_storage->dev[i].addr, sizeof(s_bt_user_storage->dev[i].addr)) &&
                memcmp(s_bt_invaild_addr, s_bt_user_storage->dev[i].addr, sizeof(s_bt_user_storage->dev[i].addr)))
        {
            int j = 0;

            for (j = 0; j < sizeof(s_bt_user_storage->dev[i].link_key) / sizeof(s_bt_user_storage->dev[i].link_key[0]); ++j)
            {
                if (s_bt_user_storage->dev[i].link_key[j] != 0 && s_bt_user_storage->dev[i].link_key[j] != 0xff)
                {
                    break;
                }
            }

            if (j < sizeof(s_bt_user_storage->dev[i].link_key) / sizeof(s_bt_user_storage->dev[i].link_key[0]))
            {
                memcpy(addr, s_bt_user_storage->dev[i].addr, sizeof(s_bt_user_storage->dev[i].addr));

                if (key)
                {
                    memcpy(key, s_bt_user_storage->dev[i].link_key, sizeof(s_bt_user_storage->dev[i].link_key));
                }

                return i;
            }
        }
    }

    return -1;
}

int32_t bluetooth_storage_find_volume_by_addr(uint8_t *addr, uint8_t *volume)
{
    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

    for (int i = 0; i < sizeof(s_bt_user_storage->dev) / sizeof(s_bt_user_storage->dev[0]); ++i)
    {
        if (!memcmp(addr, s_bt_user_storage->dev[i].addr, sizeof(s_bt_user_storage->dev[i].addr)))
        {
            if (volume)
            {
                *volume = s_bt_user_storage->dev[i].a2dp_volume;
            }

            return i;
        }
    }

    return -1;
}

int32_t bluetooth_storage_save_volume(uint8_t *addr, uint8_t volume)
{
    int32_t index = 0;

    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

    index = bluetooth_storage_find_linkkey_info_index(addr, NULL);

    if (index >= 0)
    {
        s_bt_user_storage->dev[index].a2dp_volume = volume;
    }
    else
    {
        LOGW("not find the device info, %02X:%02X:%02X:%02X:%02X:%02X",
             addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
        return -1;
    }

    return 0;
}

int32_t bluetooth_storage_find_hfp_volume_by_addr(uint8_t *addr, uint8_t *mic_vol, uint8_t *spk_vol)
{
    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

    for (int i = 0; i < sizeof(s_bt_user_storage->dev) / sizeof(s_bt_user_storage->dev[0]); ++i)
    {
        if (!memcmp(addr, s_bt_user_storage->dev[i].addr, sizeof(s_bt_user_storage->dev[i].addr)))
        {
            if (mic_vol)
            {
                *mic_vol = s_bt_user_storage->dev[i].hfp_mic_vol;
            }

            if (spk_vol)
            {
                *spk_vol = s_bt_user_storage->dev[i].hfp_spk_vol;
            }

            return i;
        }
    }

    return -1;
}

int32_t bluetooth_storage_save_hfp_volume(uint8_t *addr, uint8_t type, uint8_t volume)
{
    int32_t index = 0;

    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

    index = bluetooth_storage_find_linkkey_info_index(addr, NULL);

    if (index >= 0)
    {
        switch (type)
        {
        case 0:
            s_bt_user_storage->dev[index].hfp_mic_vol = volume;
            break;

        case 1:
            s_bt_user_storage->dev[index].hfp_spk_vol = volume;
            break;
        }
    }
    else
    {
        LOGW("not find the device info, %02X:%02X:%02X:%02X:%02X:%02X",
             addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
        return -1;
    }

    return 0;
}

int32_t bluetooth_storage_find_addr_by_hash(uint8_t *addr, uint32_t hash)
{
    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

    for (int i = 0; i < sizeof(s_bt_user_storage->dev) / sizeof(s_bt_user_storage->dev[0]); ++i)
    {
        if (hash == s_bt_user_storage->dev[i].hash)
        {
            if (addr)
            {
                os_memcpy(addr, s_bt_user_storage->dev[i].addr, 6);
            }

            return i;
        }
    }

    return -1;
}

int32_t bluetooth_storage_save_hash(uint8_t *addr, uint32_t hash)
{
    int32_t index = 0;

    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

    index = bluetooth_storage_find_linkkey_info_index(addr, NULL);

    if (index >= 0)
    {
        s_bt_user_storage->dev[index].hash = hash;
    }
    else
    {
        LOGW("not find the device info, %02X:%02X:%02X:%02X:%02X:%02X",
             addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
        return -1;
    }

    return 0;
}

int32_t bluetooth_storage_del_linkkey_info(uint8_t *addr)
{
    int32_t ret = 0;
    int32_t index = 0;
    int i = 0;

    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

    index = bluetooth_storage_find_linkkey_info_index(addr, NULL);

    if (index >= 0)
    {
        for (i = index + 1; i < sizeof(s_bt_user_storage->dev) / sizeof(s_bt_user_storage->dev[0]); ++i)
        {
            memcpy(&s_bt_user_storage->dev[i - 1], &s_bt_user_storage->dev[i], sizeof(s_bt_user_storage->dev[0]));
        }

        memset(&s_bt_user_storage->dev[i - 1], 0, sizeof(s_bt_user_storage->dev[0]));
    }

#if 0//CONFIG_EASY_FLASH_V4
    ret = ef_set_env_blob(BT_STORAGE_KEY, s_bt_user_storage, sizeof(*s_bt_user_storage));

    if (ret)
    {
        LOGE("ef_set_env_blob err %d", ret);
    }

#endif
    return ret;
}

int32_t bluetooth_storage_clean_linkkey_info(void)
{
    int32_t ret = 0;

    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

    memset(s_bt_user_storage->dev, 0, sizeof(s_bt_user_storage->dev));

#if 0//CONFIG_EASY_FLASH_V4
    ret = ef_set_env_blob(BT_STORAGE_KEY, s_bt_user_storage, sizeof(*s_bt_user_storage));

    if (ret)
    {
        LOGE("ef_set_env_blob err %d", ret);
    }

#endif
    return ret;
}

int32_t bluetooth_storage_sync_to_flash(void)
{
    int32_t ret = 0;

    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

#if CONFIG_EASY_FLASH_V4

#if 0//CONFIG_BLUETOOTH_MULTI_CONTROLLER && CONFIG_UART_SW_FLOW_CTRL
    bk_usfc_hw_set_rts(CONFIG_BLUETOOTH_BSC_UART_ID, 1);
    bk_usfc_sw_set_rts(CONFIG_BLUETOOTH_BSC_UART_ID, 1);
#endif

    ret = ef_set_env_blob(BT_STORAGE_KEY, s_bt_user_storage, sizeof(*s_bt_user_storage));

    if (ret)
    {
        LOGE("ef_set_env_blob err %d", ret);
    }

    ret = ef_save_env();

#if 0//CONFIG_BLUETOOTH_MULTI_CONTROLLER && CONFIG_UART_SW_FLOW_CTRL
    bk_usfc_sw_set_rts(CONFIG_BLUETOOTH_BSC_UART_ID, 0);
    bk_usfc_hw_set_rts(CONFIG_BLUETOOTH_BSC_UART_ID, 0);
#endif

    if (ret)
    {
        LOGE("ef_save_env err %d", ret);
    }

#endif
    return ret;
}

int32_t bluetooth_storage_linkkey_debug(void)
{
    char tmp_buff[128] = {0};
    int index = 0;

    for (int i = 0; i < sizeof(s_bt_user_storage->dev) / sizeof(s_bt_user_storage->dev[0]); ++i)
    {
        memset(tmp_buff, 0, sizeof(tmp_buff));

        index = sprintf(tmp_buff, "%02X:%02X:%02X:%02X:%02X:%02X ", s_bt_user_storage->dev[i].addr[5],
                        s_bt_user_storage->dev[i].addr[4],
                        s_bt_user_storage->dev[i].addr[3],
                        s_bt_user_storage->dev[i].addr[2],
                        s_bt_user_storage->dev[i].addr[1],
                        s_bt_user_storage->dev[i].addr[0]);

        for (int j = 0; j < sizeof(s_bt_user_storage->dev[i].link_key); ++j)
        {
            index += sprintf(tmp_buff + index, "%02X", s_bt_user_storage->dev[i].link_key[j]);
        }

        LOGI("%s", tmp_buff);
    }
#if CONFIG_BLE
    for (int i = 0; i < sizeof(s_bt_user_storage->dev) / sizeof(s_bt_user_storage->dev[0]); ++i)
    {
        bk_ble_bond_dev_t *bond = &s_bt_user_storage->dev[i].ble_key;

        memset(tmp_buff, 0, sizeof(tmp_buff));
        index = sprintf(tmp_buff, "%02X:%02X:%02X:%02X:%02X:%02X llink_key: ", bond->bd_addr[5],
                        bond->bd_addr[4],
                        bond->bd_addr[3],
                        bond->bd_addr[2],
                        bond->bd_addr[1],
                        bond->bd_addr[0]);
        for (int j = 0; j < sizeof(bond->bond_key.llink_key.key); ++j)
        {
            index += sprintf(tmp_buff + index, "%02X", bond->bond_key.llink_key.key[j]);
        }
        os_printf("%s %s\n", __func__, tmp_buff);
        memset(tmp_buff, 0, sizeof(tmp_buff));
        index = sprintf(tmp_buff, "%02X:%02X:%02X:%02X:%02X:%02X ltk: ", bond->bd_addr[5],
                        bond->bd_addr[4],
                        bond->bd_addr[3],
                        bond->bd_addr[2],
                        bond->bd_addr[1],
                        bond->bd_addr[0]);
        for (int j = 0; j < sizeof(bond->bond_key.lenc_key.ltk); ++j)
        {
            index += sprintf(tmp_buff + index, "%02X", bond->bond_key.lenc_key.ltk[j]);
        }
        os_printf("%s %s\n", __func__, tmp_buff);
    }
#endif
    return 0;
}

#if CONFIG_BLE

int32_t bluetooth_storage_save_ble_key_info(bk_ble_bond_dev_t *list, uint32_t count)
{
    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

    /* Full-list overwrite: drop every stored BLE bond first, then attach each
     * incoming bond to the device record matching its address, allocating a
     * BLE-only record (addr = BLE addr, link_key all-zero) when the peer has no
     * BR/EDR link key. A peer whose BLE and BR/EDR addresses match shares one
     * slot with its link-key record. */
    for (int i = 0; i < sizeof(s_bt_user_storage->dev) / sizeof(s_bt_user_storage->dev[0]); ++i)
    {
        os_memset(&s_bt_user_storage->dev[i].ble_key, 0, sizeof(s_bt_user_storage->dev[i].ble_key));
    }

    for (uint32_t i = 0; i < count; ++i)
    {
        int32_t index;
        uint8_t record_addr[6];

        if (!bluetooth_storage_is_addr_valid(list[i].bd_addr))
        {
            continue;
        }

        os_memcpy(record_addr, list[i].bd_addr, sizeof(record_addr));
        if ((list[i].bond_key.key_mask & BK_LE_KEY_PID) &&
                bluetooth_storage_is_addr_valid(list[i].bond_key.pid_key.static_addr))
        {
            os_memcpy(record_addr, list[i].bond_key.pid_key.static_addr, sizeof(record_addr));
        }

        index = bluetooth_storage_find_linkkey_info_index(record_addr, NULL);
        if (index < 0)
        {
            index = bluetooth_storage_alloc_linkkey_info(record_addr);
        }
        if (index < 0)
        {
            LOGW("no slot for %02X:%02X:%02X:%02X:%02X:%02X",
                 record_addr[5], record_addr[4], record_addr[3],
                 record_addr[2], record_addr[1], record_addr[0]);
            continue;
        }

        os_memcpy(&s_bt_user_storage->dev[index].ble_key, &list[i], sizeof(list[i]));
    }

    return 0;
}

int32_t bluetooth_storage_clean_ble_key_info(void)
{
    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

    for (int i = 0; i < sizeof(s_bt_user_storage->dev) / sizeof(s_bt_user_storage->dev[0]); ++i)
    {
        os_memset(&s_bt_user_storage->dev[i].ble_key, 0, sizeof(s_bt_user_storage->dev[i].ble_key));
    }

    return 0;
}

int32_t bluetooth_storage_read_ble_key_info(bk_ble_bond_dev_t *list, uint32_t *count)
{
    uint32_t out = 0;

    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

    /* Gather the valid BLE bonds scattered across the per-device records, packed
     * to the front of the caller's list and capped at the caller's capacity. */
    for (int i = 0; i < sizeof(s_bt_user_storage->dev) / sizeof(s_bt_user_storage->dev[0]) && out < *count; ++i)
    {
        if (bluetooth_storage_is_addr_valid(s_bt_user_storage->dev[i].ble_key.bd_addr))
        {
            os_memcpy(&list[out], &s_bt_user_storage->dev[i].ble_key, sizeof(list[out]));
            out++;
        }
    }

    *count = out;

    return 0;
}

int32_t bluetooth_storage_find_ble_key_info_index(uint8_t *addr)
{
    if (!s_bt_user_storage)
    {
        os_printf("%s not init\n", __func__);
        return -1;
    }

    for (int i = 0; i < (int)(sizeof(s_bt_user_storage->dev) / sizeof(s_bt_user_storage->dev[0])); ++i)
    {
        bk_ble_bond_dev_t *bond = &s_bt_user_storage->dev[i].ble_key;

        if (memcmp(s_bt_empty_addr, bond->bd_addr, sizeof(bond->bd_addr)) == 0 ||
            memcmp(s_bt_invaild_addr, bond->bd_addr, sizeof(bond->bd_addr)) == 0)
        {
            continue;
        }

        if (!memcmp(addr, bond->bd_addr, sizeof(bond->bd_addr)))
        {
            return i;
        }

        if ((bond->bond_key.key_mask & BK_LE_KEY_PID) &&
            !memcmp(addr, bond->bond_key.pid_key.static_addr, sizeof(bond->bond_key.pid_key.static_addr)))
        {
            return i;
        }
    }

    return -1;
}

int32_t bluetooth_storage_has_ble_ltk_for_addr(uint8_t *addr)
{
    int32_t index = bluetooth_storage_find_ble_key_info_index(addr);

    if (index < 0)
    {
        return 0;
    }

    return ((s_bt_user_storage->dev[index].ble_key.bond_key.key_mask & BK_LE_KEY_LENC) != 0) ? 1 : 0;
}

int32_t bluetooth_storage_save_local_key(bk_ble_local_keys_t *key)
{
    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

    os_memcpy(&s_bt_user_storage->local_keys, key, sizeof(*key));

    return 0;
}

int32_t bluetooth_storage_clean_local_key(void)
{
    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

    os_memset(&s_bt_user_storage->local_keys, 0, sizeof(s_bt_user_storage->local_keys));

    return 0;
}

int32_t bluetooth_storage_read_local_key(bk_ble_local_keys_t *key)
{
    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return -1;
    }

    os_memcpy(key, &s_bt_user_storage->local_keys, sizeof(*key));

    return 0;
}

#endif

static uint8_t bluetooth_storage_is_addr_valid(uint8_t *addr)
{
    uint8_t sum_ff = 0xff, sum_zero = 0;

    for (int i = 0; i < 6; ++i)
    {
        sum_ff &= addr[i];
        sum_zero |= addr[i];
    }

    return sum_ff != 0xff && sum_zero != 0;
}

uint8_t bluetooth_storage_get_bond_device_num(void)
{
    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return 0;
    }

    uint8_t count = 0;

    for (int i = 0; i < sizeof(s_bt_user_storage->dev) / sizeof(s_bt_user_storage->dev[0]); ++i)
    {
        if (bluetooth_storage_is_addr_valid(s_bt_user_storage->dev[i].addr))
        {
            count++;
        }
    }

    return count;
}

uint32_t bluetooth_storage_get_bond_hash(uint16_t hasharray[], uint32_t arraylen )
{
    if (!s_bt_user_storage)
    {
        LOGE("not init");
        return 0;
    }

    uint8_t count = 0;

    for (int i = 0; i < sizeof(s_bt_user_storage->dev) / sizeof(s_bt_user_storage->dev[0]); ++i)
    {
        if (bluetooth_storage_is_addr_valid(s_bt_user_storage->dev[i].addr))
        {
            uint16_t temp_hash = s_bt_user_storage->dev[i].hash;
            os_memcpy(&hasharray[count], &temp_hash, sizeof(temp_hash));
            count++;

            if (count == arraylen)
            {
                break;
            }
        }
    }

    return count;
}

int32_t bluetooth_storage_init(void)
{
    int32_t ret = 0;
    size_t saved_len = 0;

    if (!s_bt_user_storage)
    {
        s_bt_user_storage = os_malloc(sizeof(*s_bt_user_storage));

        if (!s_bt_user_storage)
        {
            LOGE("alloc fail");
            return -1;
        }

        os_memset(s_bt_user_storage, 0, sizeof(*s_bt_user_storage));

#if CONFIG_EASY_FLASH_V4
        ret = ef_get_env_blob(BT_STORAGE_KEY, s_bt_user_storage, sizeof(*s_bt_user_storage), &saved_len);

        if (ret != sizeof(*s_bt_user_storage) || saved_len != sizeof(*s_bt_user_storage))
        {
            LOGW("ef_get_env_blob err %d %d", ret, saved_len);

            if (saved_len < sizeof(*s_bt_user_storage))
            {
                os_memset(((uint8_t *)s_bt_user_storage) + saved_len, 0, sizeof(*s_bt_user_storage) - saved_len);
            }
        }

#endif
    }

    (void)saved_len;
    (void)ret;
    return 0;
}

int32_t bluetooth_storage_deinit(void)
{
    if (s_bt_user_storage)
    {
        os_free(s_bt_user_storage);
        s_bt_user_storage = NULL;
    }

    return 0;
}
