#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <components/bk_frame_buffer.h>
#include <stdint.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"

/* Brings in SOC_USB_HS_BASE for the MUSB register dump. The SDK
 * port file usb_dc_beken_musb_mhdrc.c uses the same header. */
#if CONFIG_SOC_BK7259
#include <soc/bk7259/reg_base.h>
#endif

#if CONFIG_FATFS
#include "ff.h"
#endif

extern void sys_ana_usb_phy_op(uint8_t en);
extern int msc_storage_init(void);

static beken_thread_t s_msc_init_thread = NULL;

#define USB_MSC_INIT_TASK_PRIORITY    BEKEN_DEFAULT_WORKER_PRIORITY
#define USB_MSC_INIT_TASK_STACK_SIZE  (1024 * 16)
#define USB_MSC_INIT_TASK_NAME        "msc_init"

#define TAG "udisk"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

bk_err_t usb_storage_enable(void)
{
    /* (c) Turn on USB PHY VCC 1.8V (ana_reg14 bit 10) + VCC 3V
     *     (ana_reg14 bit 11). This is the ONLY thing that brings
     *     up the analog rail on BK7259. */
    //LOGI("U-disk: enabling USB VCC 3V + VCC 1.8V via sys_ana_usb_phy_op(1)\n");
    //LOGI("        (ana_reg14 @0x%08lx |= bit10|bit11)\n",
    //     (unsigned long)(0x44010000UL + 0x4eUL * 4UL));
    //sys_ana_usb_phy_op(1);

    /* 4) Register the MSC class on top of the device controller.
     *    This also runs the SD-NAND bring-up via the V3P0 backend
     *    (gated by MSC_SD_BACKEND_AVAILABLE, see input.txt
     *    section 11.2). */
    LOGI("U-disk: calling msc_storage_init()\n");
    msc_storage_init();

    return BK_OK;
}

static void msc_storage_init_task(void *arg)
{
    (void)arg;

    bk_err_t ret = 0;

    GPIO_DOWN(GPIO_32);
    GPIO_DOWN(GPIO_33);
    GPIO_DOWN(GPIO_34);
    GPIO_DOWN(GPIO_35);
    GPIO_DOWN(GPIO_36);
    GPIO_DOWN(GPIO_37);
    GPIO_DOWN(GPIO_38);
    GPIO_DOWN(GPIO_39);
    GPIO_DOWN(GPIO_40);
    GPIO_DOWN(GPIO_41);
    GPIO_DOWN(GPIO_42);
    GPIO_DOWN(GPIO_43);
    GPIO_DOWN(GPIO_44);
    GPIO_DOWN(GPIO_45);
    GPIO_DOWN(GPIO_46);
    GPIO_DOWN(GPIO_47);
    GPIO_DOWN(GPIO_48);

    LOGI("[%s] msc_init: ENTER\r\n", TAG);
    ret = usb_storage_enable();
    LOGI("[%s] msc_init: RET=%d (see [udisk]/[usb-dbg] lines for detail)\r\n",
              TAG, ret);

    /* Bring-up done. The CherryUSB "usbd_msc" worker thread keeps
     * the MSC pipeline alive on its own; the usb_dbg watchdog
     * (spawned inside usb_storage_enable()) handles "is it still
     * running?" diagnostics from here on out. This task has no
     * steady-state work, so self-delete to release its stack/TCB
     * back to FreeRTOS.
     *
     * NOTE: do NOT print a "still alive +Ns" heartbeat here -- the
     * usb_dbg watchdog already does that, every 2 s for up to 30 s,
     * with much richer information (FADDR, INTRUSB, irq_cnt, PHY
     * VCC bits, ...). A second heartbeat would just interleave with
     * those dumps and add no signal. */
    LOGI("[%s] msc_init: task done, self-deleting\r\n", TAG);
    s_msc_init_thread = NULL;
    rtos_delete_thread(NULL);
}

int main(void)
{
    bk_init();

    LOGI("M55 main running...\r\n");

    bk_err_t cret = rtos_create_thread(&s_msc_init_thread,
        USB_MSC_INIT_TASK_PRIORITY,
        USB_MSC_INIT_TASK_NAME,
        (beken_thread_function_t)msc_storage_init_task,
        USB_MSC_INIT_TASK_STACK_SIZE,
        NULL);
    if (cret != BK_OK) {
        LOGE("[%s] create %s task FAILED ret=%d, skipping MSC bring-up\r\n", TAG, USB_MSC_INIT_TASK_NAME, cret);
        return -1;
   }

    return 0;
}
