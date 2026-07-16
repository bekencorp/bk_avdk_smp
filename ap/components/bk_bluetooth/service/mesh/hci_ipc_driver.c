#include <errno.h>
#include <stdint.h>

#include <components/log.h>
#include <drivers/bluetooth/hci_driver.h>
#include <bluetooth/buf.h>
#include <bluetooth/hci.h>
#include <net/buf.h>
#include <sys/byteorder.h>

#include "bt_ipc_core.h"

#define TAG "mesh_hci"

static uint8_t s_mesh_hci_driver_registered;

static void mesh_hci_ipc_recv_event(uint8_t *data, uint16_t len)
{
    struct net_buf *buf;
    struct bt_hci_evt_hdr *evt;
    uint8_t flags;

    if (len < sizeof(*evt))
    {
        BK_LOGW(TAG, "short evt len=%u\n", len);
        return;
    }

    evt = (struct bt_hci_evt_hdr *)data;
    if (len < sizeof(*evt) + evt->len)
    {
        BK_LOGW(TAG, "bad evt len=%u param=%u\n", len, evt->len);
        return;
    }

    flags = bt_hci_evt_get_flags(evt->evt);
    if (flags & BT_HCI_EVT_FLAG_RECV_PRIO)
    {
        buf = bt_buf_get_cmd_complete(K_NO_WAIT);
    }
    else
    {
        buf = bt_buf_get_rx(BT_BUF_EVT, K_NO_WAIT);
    }

    if (!buf)
    {
        BK_LOGW(TAG, "no evt buf evt=0x%02x len=%u\n", evt->evt, len);
        return;
    }

    net_buf_add_mem(buf, data, sizeof(*evt) + evt->len);
    bt_buf_set_type(buf, BT_BUF_EVT);
    bt_recv(buf);
}

static void mesh_hci_ipc_recv_acl(uint8_t *data, uint16_t len)
{
    struct net_buf *buf;
    struct bt_hci_acl_hdr *acl;
    uint16_t acl_len;

    if (len < sizeof(*acl))
    {
        BK_LOGW(TAG, "short acl len=%u\n", len);
        return;
    }

    acl = (struct bt_hci_acl_hdr *)data;
    acl_len = sys_le16_to_cpu(acl->len);

    if (len < sizeof(*acl) + acl_len)
    {
        BK_LOGW(TAG, "bad acl len=%u payload=%u\n", len, acl_len);
        return;
    }

    buf = bt_buf_get_rx(BT_BUF_ACL_IN, K_NO_WAIT);
    if (!buf)
    {
        BK_LOGW(TAG, "no acl buf len=%u\n", len);
        return;
    }

    net_buf_add_mem(buf, data, sizeof(*acl) + acl_len);
    bt_recv(buf);
}

static void mesh_hci_ipc_recv(uint8_t *data, uint16_t len)
{
    if (!data || len < 1)
    {
        BK_LOGW(TAG, "invalid rx data=%p len=%u\n", data, len);
        return;
    }

    switch (data[0])
    {
    case HCI_EVENT_PKT:
        mesh_hci_ipc_recv_event(data + 1, len - 1);
        break;
    case HCI_ACL_DATA_PKT:
        mesh_hci_ipc_recv_acl(data + 1, len - 1);
        break;
    default:
        BK_LOGW(TAG, "unsupported rx type=%u len=%u\n", data[0], len);
        break;
    }
}

static int mesh_hci_driver_open(void)
{
    bt_ipc_register_hci_send_callback(mesh_hci_ipc_recv);
    return 0;
}

static int mesh_hci_driver_send(struct net_buf *buf)
{
    int ret = 0;

    if (!buf)
    {
        return -EINVAL;
    }

    switch (bt_buf_get_type(buf))
    {
    case BT_BUF_CMD:
    {
        struct bt_hci_cmd_hdr *cmd = (struct bt_hci_cmd_hdr *)buf->data;

        if (buf->len < sizeof(*cmd) || buf->len < sizeof(*cmd) + cmd->param_len)
        {
            BK_LOGW(TAG, "bad cmd buf len=%u\n", buf->len);
            ret = -EINVAL;
            break;
        }

        bt_ipc_hci_send_cmd(sys_le16_to_cpu(cmd->opcode), buf->data + sizeof(*cmd), cmd->param_len);
        break;
    }
    case BT_BUF_ACL_OUT:
    {
        struct bt_hci_acl_hdr *acl = (struct bt_hci_acl_hdr *)buf->data;
        uint16_t acl_len;

        if (buf->len < sizeof(*acl))
        {
            BK_LOGW(TAG, "bad acl buf len=%u\n", buf->len);
            ret = -EINVAL;
            break;
        }

        acl_len = sys_le16_to_cpu(acl->len);
        if (buf->len < sizeof(*acl) + acl_len)
        {
            BK_LOGW(TAG, "bad acl payload len=%u payload=%u\n", buf->len, acl_len);
            ret = -EINVAL;
            break;
        }

        bt_ipc_hci_send_acl_data(sys_le16_to_cpu(acl->handle), buf->data + sizeof(*acl), acl_len);
        break;
    }
    default:
        BK_LOGW(TAG, "unsupported tx type=%u\n", bt_buf_get_type(buf));
        ret = -EINVAL;
        break;
    }

    net_buf_unref(buf);
    return ret;
}

static const struct bt_hci_driver s_mesh_hci_driver =
{
    .name = "bk7259_mesh_ipc",
    .bus = BT_HCI_DRIVER_BUS_VIRTUAL,
    .open = mesh_hci_driver_open,
    .send = mesh_hci_driver_send,
};

int bk_mesh_hci_ipc_driver_init(void)
{
    int ret;

    if (s_mesh_hci_driver_registered)
    {
        return 0;
    }

    ret = bt_hci_driver_register(&s_mesh_hci_driver);
    if (ret == 0 || ret == -EALREADY)
    {
        s_mesh_hci_driver_registered = 1;
        return 0;
    }

    BK_LOGW(TAG, "driver register failed ret=%d\n", ret);
    return ret;
}

/* Detach the IPC RX callback, mirroring hal_hci_driver_close() used by the
 * RW/Ethermind hosts. Zephyr 2.7.6 has no bt_hci_driver_unregister(), so the
 * registered driver struct (bt_dev.drv) stays as-is; only the IPC transport
 * callback is released here. */
int bk_mesh_hci_ipc_driver_deinit(void)
{
    if (!s_mesh_hci_driver_registered)
    {
        return 0;
    }

    bt_ipc_register_hci_send_callback(NULL);

    return 0;
}
