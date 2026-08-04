#include "bk_private/bk_init.h"
#include <components/log.h>
#include <os/os.h>
#include "components/bluetooth/bk_dm_bluetooth.h"

#define TAG "dut_main"

extern void dut_test_init(void);

int main(void)
{
    bk_err_t ret;

    bk_init();

    /*
     * Register the project command only after AP CLI and the Bluetooth host
     * have completed their normal bk_init() startup sequence.
     */
    rtos_delay_milliseconds(500);

    ret = bk_bluetooth_init();
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "Bluetooth initialization failed: %d\n", ret);
        return ret;
    }

    dut_test_init();
    return 0;
}
