#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>

#include <common/bk_include.h>
#include <components/log.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_hardware_ram.h>

#include <avdk_error.h>
#include <driver/isp.h>
#include <driver/io_matrix.h>

#include "vsi_comm_video.h"
#include "vsi_comm_isp.h"
#include "vsi_comm_sns.h"
#include <modules/private/veri_isp/mpi_isp.h>
#include <modules/veri_isp/flexa_sync.h>
#include "mpi_isp_wb.h"
#include "vsi_comm_awb.h"
#include "sys_hal.h"
#include <driver/int.h>
#include <driver/sys_pm.h>
#include "sys_driver.h"
// #include "media_reg.h"

#include <driver/gpio.h>
#include "gpio_driver.h"
#include "spinlock.h"
#define TAG "isp_core"

#include "avdk_monitor.h"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct {
    isp_isr_t isr_handler;
    void *param;
    uint8_t enable : 1;
    uint8_t reg_en : 1;
} isp_isr_handler_t;

enum {
    ISP_FSM_INIT = 0,
    ISP_FSM_CHN_ENABLE,
    ISP_FSM_ENABLE,
} isp_fsm_t;

#define FLEXA_LINES 16
#define ISP_FLEXA_STREAM_ID_Y 0x14
#define ISP_FLEXA_STREAM_ID_CB 0x15
#define ISP_FLEXA_STREAM_ID_CR 0x16

static isp_isr_handler_t isp_isr_handler[ISP_ISR_MAX][ISP_ISR_MODULE_MAX] = {0};
static uint8_t s_isp_clk_vote_cnt = 0;
#if CONFIG_SOC_SMP
static SPINLOCK_SECTION volatile spinlock_t s_isp_isr_spin_lock = SPIN_LOCK_INIT;
#endif

static inline uint32_t isp_isr_lock_irqsave(void)
{
    uint32_t irq_flags = rtos_disable_int();

#if CONFIG_SOC_SMP
    spin_lock(&s_isp_isr_spin_lock);
#endif

    return irq_flags;
}

static inline void isp_isr_unlock_irqrestore(uint32_t irq_flags)
{
#if CONFIG_SOC_SMP
    spin_unlock(&s_isp_isr_spin_lock);
#endif

    rtos_enable_int(irq_flags);
}

#ifdef ISP_AE_V10
#include "mpi_isp_ae.h"
#endif

#ifdef VSI_AE_ALGO
extern ISP_AE_FUNC_S vsiAeAlgo;
#endif

#ifdef VSI_AWB_ALGO
extern ISP_AWB_FUNC_S vsiAwbAlgo;
#endif

void bk_mipi_csi_ext_set_enable(uint8_t mode);
int VSI_MPI_ISP_SetScaleAttr(ISP_CHN IspChn, ISP_CHN_ATTR_S *pChnAttr);

static void isp_unregister_sensor_callbacks(isp_control_t *control)
{
    uint8_t i;

    if (control == NULL || !control->sensor_sns_registered)
    {
        return;
    }

    for (i = 0; i < ISP_PORT_CNT; i++)
    {
        ISP_PUB_ATTR_S *p = (ISP_PUB_ATTR_S *)control->pub_attr[i];

        if (p != NULL && p->ispInputType == INPUT_TYPE_SENSOR)
        {
            ISP_PORT IspPort = control->port;

            IspPort.portId = i;
            VSI_MPI_ISP_SnsUnRegCallBack(IspPort);
        }
    }

    control->sensor_sns_registered = 0;
}

int isp_set_port_attribute(ISP_PORT IspPort, ISP_PUB_ATTR_S *pPubAttr)
{
    int ret;

    if (pPubAttr->ispInputType == INPUT_TYPE_SENSOR) {

        ret = VSI_MPI_ISP_SnsRegCallBack(IspPort, pPubAttr->pSnsObj, 8);
        if (ret) {
            return ret;
        }
#ifdef VSI_AE_ALGO
        ret = VSI_MPI_ISP_AeRegCallBack(IspPort, &vsiAeAlgo);
        if (ret) {
            return ret;
        }
#endif

#ifdef VSI_AWB_ALGO
        ret = VSI_MPI_ISP_AwbRegCallBack(IspPort, &vsiAwbAlgo);
        if (ret) {
            return ret;
        }
#endif
    }

    ISP_PORT_ATTR_S portAttr;
    VSI_MPI_ISP_GetPortAttr(IspPort, &portAttr);

    portAttr.ispInputType              = pPubAttr->ispInputType;
    portAttr.ispMode                   = pPubAttr->ispMode;
    portAttr.hdrMode                   = pPubAttr->hdrMode;
    portAttr.stichMode                 = pPubAttr->stichMode;
    portAttr.pixelFormat               = pPubAttr->pixelFormat;
    portAttr.snsFps                    = pPubAttr->snsFps;
    portAttr.snsRect.width          = pPubAttr->snsRect.width;
    portAttr.snsRect.height         = pPubAttr->snsRect.height;
    portAttr.inFormRect.top         = pPubAttr->inFormRect.top;
    portAttr.inFormRect.left        = pPubAttr->inFormRect.left;
    portAttr.inFormRect.width       = pPubAttr->inFormRect.width;
    portAttr.inFormRect.height      = pPubAttr->inFormRect.height;
    portAttr.outFormRect.top        = pPubAttr->outFormRect.top;
    portAttr.outFormRect.left       = pPubAttr->outFormRect.left;
    portAttr.outFormRect.width      = pPubAttr->outFormRect.width;
    portAttr.outFormRect.height     = pPubAttr->outFormRect.height;
    portAttr.iSRect.top             = pPubAttr->iSRect.top;
    portAttr.iSRect.left            = pPubAttr->iSRect.left;
    portAttr.iSRect.width           = pPubAttr->iSRect.width;
    portAttr.iSRect.height          = pPubAttr->iSRect.height;

    ret = VSI_MPI_ISP_SetPortAttr(IspPort, &portAttr);
    if (ret) {
        LOGE("%s, %d, VSI_MPI_ISP_SetPortAttr failed\n", __func__, __LINE__);
        return ret;
    }

    return VSI_SUCCESS;
}


static void isp_isr_callback(uint32_t state, void *args)
{
    isp_control_t *control = (isp_control_t *)args;

    if (control && control->state == ISP_FSM_CHN_ENABLE)
    {
        if (state & 0x0C)
        {
            static uint32_t error_count = 0;
            error_count++;
            if (error_count > 1000) {
                LOGW("%s, %d, error state: %d\n", __func__, __LINE__, state);
                error_count = 0;
            }
        }
    }
}

static void isp_isr_callback_ext(vsi_u32_t state, void *args)
{
    uint32_t state_temp = (uint32_t)state;
    isp_isr_callback(state_temp, args);
}

static bool isp_peer_channel_warmup_done(isp_control_t *control, uint8_t chnl_id)
{
    uint8_t peer_id = (chnl_id == ISP_MP_CHN_ID) ? ISP_SP_CHN_ID : ISP_MP_CHN_ID;

    /* Only a peer that actually finished a warmup skip (warmup_done latched)
     * proves the shared sensor/AE is stable. A peer that merely has
     * skip_frames==0 (opted out, e.g. SP by default) is NOT an authority and
     * must not short-circuit the warming channel. */
    return control->chn[peer_id].enable
        && control->chn[peer_id].warmup_done;
}

static bool isp_channel_skip_warmup_needed(isp_control_t *control, uint8_t chnl_id)
{
    if (control->chn[chnl_id].skip_frames_remaining == 0)
    {
        return false;
    }

    /* MP/SP share sensor/AE: late open after peer warmup must not skip again. */
    if (isp_peer_channel_warmup_done(control, chnl_id))
    {
        return false;
    }

    return true;
}

static void isp_mi_isr_callback_handle(isp_control_t *control, uint8_t isr_type, uint8_t chnl_id, uint8_t ok)
{
    uint8_t i = 0;

    if (control->chn[chnl_id].skip_active)
    {
        if (isr_type == ISP_FRAME_END_DONE && ok)
        {
            if (control->chn[chnl_id].skip_frames_remaining > 0)
            {
                control->chn[chnl_id].skip_frames_remaining--;
            }
            if (control->chn[chnl_id].skip_frames_remaining == 0)
            {
                /* Finished a real warmup countdown: this channel is now a
                 * warmup authority for its peer (shared sensor/AE is stable). */
                control->chn[chnl_id].warmup_done = 1;
            }
            control->chn[chnl_id].skip_active = 0;
        }
        return;
    }

    if (isr_type == ISP_MB_LINE_DONE)
    {
        for (i = 0; i < ISP_ISR_MODULE_MAX; i++)
        {
            if (isp_isr_handler[isr_type][i].reg_en)
            {
                if (isp_isr_handler[isr_type][i].enable == false
                    && control->chn[chnl_id].line == 1)
                {
                    isp_isr_handler[isr_type][i].enable = true;
                }

                /* deregister race: bk_isp_deregister_isr_callback memsets this
                 * slot under spinlock while ISR runs lock-free. NULL-check
                 * isr_handler so we degrade to a no-op instead of jumping to 0. */
                if (isp_isr_handler[isr_type][i].enable
                    && isp_isr_handler[isr_type][i].isr_handler != NULL)
                {
                    isp_isr_handler[isr_type][i].isr_handler(control->chn[chnl_id].sequence,
                        control->chn[chnl_id].line, chnl_id, ok,
                        isp_isr_handler[isr_type][i].param);
                }
            }
        }
    }
    else
    {
        for (i = 0; i < ISP_ISR_MODULE_MAX; i++)
        {
            if (isp_isr_handler[isr_type][i].reg_en)
            {
                if (isp_isr_handler[isr_type][i].isr_handler)
                {
                    isp_isr_handler[isr_type][i].isr_handler(control->chn[chnl_id].sequence,
                        control->chn[chnl_id].line, chnl_id, ok,
                        isp_isr_handler[isr_type][i].param);
                }
            }
        }
    }
}

static void isp_mi_isr_callback(uint32_t state, void *args)
{
    uint8_t i = 0;
    isp_control_t *control = (isp_control_t *)args;

    //LOGD("%s, %d, %d\n", __func__, __LINE__, state);

    if (control && control->state == ISP_FSM_CHN_ENABLE)
    {
        if (state & ISP_MP_MB_LINE_STATE)
        {

            if (control->chn[ISP_MP_CHN_ID].line == 0)
            {
                ISP_MP_FRAME_START();
                control->chn[ISP_MP_CHN_ID].skip_active =
                    isp_channel_skip_warmup_needed(control, ISP_MP_CHN_ID) ? 1 : 0;
            }

            ISP_MP_LINE_START();

            control->chn[ISP_MP_CHN_ID].line++;
            AVDK_MONITOR_MP_LINE_PLUS();
            isp_mi_isr_callback_handle(control, ISP_MB_LINE_DONE, ISP_MP_CHN_ID, true);

            ISP_MP_LINE_END();
        }

        if (state & ISP_SP_MB_LINE_STATE)
        {
            if (control->chn[ISP_SP_CHN_ID].line == 0)
            {
                ISP_SP_FRAME_START();
                control->chn[ISP_SP_CHN_ID].skip_active =
                    isp_channel_skip_warmup_needed(control, ISP_SP_CHN_ID) ? 1 : 0;
            }

            ISP_SP_LINE_START();

            control->chn[ISP_SP_CHN_ID].line++;
            AVDK_MONITOR_SP_LINE_PLUS();
            isp_mi_isr_callback_handle(control, ISP_MB_LINE_DONE, ISP_SP_CHN_ID, true);

            ISP_SP_LINE_END();
        }

        if (state & ISP_MP_FRAME_END_STATE)
        {
            ISP_MP_LINE_START();
            control->chn[ISP_MP_CHN_ID].line++;
            AVDK_MONITOR_MP_FRAME_PLUS();

            if (control->chn[ISP_MP_CHN_ID].enable_flexa)
            {
                if (control->chn[ISP_MP_CHN_ID].line == control->chn[ISP_MP_CHN_ID].total_line)
                {
                    isp_mi_isr_callback_handle(control, ISP_MB_LINE_DONE, ISP_MP_CHN_ID, true);
                    isp_mi_isr_callback_handle(control, ISP_FRAME_END_DONE, ISP_MP_CHN_ID, true);
                }
                else
                {
                    isp_mi_isr_callback_handle(control, ISP_MB_LINE_DONE, ISP_MP_CHN_ID, false);
                    isp_mi_isr_callback_handle(control, ISP_FRAME_END_DONE, ISP_MP_CHN_ID, false);
                }
            }
            else
            {
                isp_mi_isr_callback_handle(control, ISP_MB_LINE_DONE, ISP_MP_CHN_ID, true);
                isp_mi_isr_callback_handle(control, ISP_FRAME_END_DONE, ISP_MP_CHN_ID, true);
            }

            control->chn[ISP_MP_CHN_ID].sequence++;
            control->chn[ISP_MP_CHN_ID].line = 0;
            ISP_MP_LINE_END();

            ISP_MP_FRAME_END();
        }

        if (state & ISP_SP_FRAME_END_STATE)
        {
            control->chn[ISP_SP_CHN_ID].line++;
            AVDK_MONITOR_SP_FRAME_PLUS();

            ISP_SP_LINE_START();

            if (control->chn[ISP_SP_CHN_ID].enable_flexa)
            {
                if (control->chn[ISP_SP_CHN_ID].line == control->chn[ISP_SP_CHN_ID].total_line)
                {
                    isp_mi_isr_callback_handle(control, ISP_MB_LINE_DONE, ISP_SP_CHN_ID, true);
                    isp_mi_isr_callback_handle(control, ISP_FRAME_END_DONE, ISP_SP_CHN_ID, true);
                }
                else
                {
                    isp_mi_isr_callback_handle(control, ISP_MB_LINE_DONE, ISP_SP_CHN_ID, false);
                    isp_mi_isr_callback_handle(control, ISP_FRAME_END_DONE, ISP_SP_CHN_ID, false);
                }
            }
            else
            {
                isp_mi_isr_callback_handle(control, ISP_MB_LINE_DONE, ISP_SP_CHN_ID, true);
                isp_mi_isr_callback_handle(control, ISP_FRAME_END_DONE, ISP_SP_CHN_ID, true);
            }

            control->chn[ISP_SP_CHN_ID].sequence++;
            control->chn[ISP_SP_CHN_ID].line = 0;

            ISP_SP_LINE_END();
            ISP_SP_FRAME_END();
        }
    }
}

static void isp_mi_isr_callback_ext(vsi_u32_t state, void *args)
{
    uint32_t state_temp = (uint32_t)state;
    isp_mi_isr_callback(state_temp, args);
}

bk_err_t bk_isp_clock_enable(uint8_t enable)
{
    bool do_pwr_up = false;
    bool do_pwr_down = false;
    uint8_t vote;
    GLOBAL_INT_DECLARATION();

    /* Ref-counted CISP clock vote shared by ISP driver and camera bus.
     * First enable powers the clock up; last disable powers it down. */
    GLOBAL_INT_DISABLE();
    if (enable)
    {
        if (s_isp_clk_vote_cnt == 0)
        {
            do_pwr_up = true;
        }
        if (s_isp_clk_vote_cnt < 0xFF)
        {
            s_isp_clk_vote_cnt++;
        }
        vote = s_isp_clk_vote_cnt;
    }
    else
    {
        if (s_isp_clk_vote_cnt == 0)
        {
            GLOBAL_INT_RESTORE();
            LOGW("%s: unbalanced disable, vote already 0\n", __func__);
            return BK_OK;
        }
        s_isp_clk_vote_cnt--;
        if (s_isp_clk_vote_cnt == 0)
        {
            do_pwr_down = true;
        }
        vote = s_isp_clk_vote_cnt;
    }
    GLOBAL_INT_RESTORE();

    if (do_pwr_up)
    {
        bk_pm_clock_ctrl(PM_CLK_ID_CISP, PM_CLK_CTRL_PWR_UP);
    }
    else if (do_pwr_down)
    {
        bk_pm_clock_ctrl(PM_CLK_ID_CISP, PM_CLK_CTRL_PWR_DOWN);
    }

    LOGD("%s: %s, vote=%u\n", __func__, enable ? "enable" : "disable", vote);
    return BK_OK;
}

bk_err_t bk_cis_auxs_clock_enable(uint32_t clk, uint32_t gpio, uint8_t enable)
{
    gpio_id_t pin = (gpio_id_t)gpio;

    (void)clk;

    if (enable)
    {
        #if CONFIG_USR_GPIO_CFG_EN
        gpio_dev_map_by_func(GPIO_DEV_CLK_AUXS_CIS);
        #endif

        // sel 1, 240MHz, div 10
        sys_drv_cis_auxs_cksel_set(CKSEL_CIS_AUXS_240M);
        sys_drv_cis_auxs_clkdiv_set(9);

        // enable csi auxs clock
        bk_pm_clock_ctrl(PM_CLK_ID_CSI, PM_CLK_CTRL_PWR_UP);
    }
    else
    {
        // disable csi auxs clock
        bk_pm_clock_ctrl(PM_CLK_ID_CSI, PM_CLK_CTRL_PWR_DOWN);
        #if CONFIG_USR_GPIO_CFG_EN
        gpio_dev_unmap_by_func(GPIO_DEV_CLK_AUXS_CIS);
        #endif
    }
    return BK_OK;
}

bk_err_t bk_cis_mclk_clock_enable(uint32_t clk, uint8_t gpio, uint8_t enable)
{
    gpio_id_t pin = (gpio_id_t)gpio;

    (void)clk;

    if (enable)
    {
        #if CONFIG_USR_GPIO_CFG_EN
        gpio_dev_map_by_func(GPIO_DEV_JPEG_MCLK);
        #endif

        // csi mclk clock configuration, default 24MHz
        // default sel 1, 240MHz div 10
        sys_drv_cis_mclk_cksel_clkdiv_set(CKSEL_CIS_MCLK_240M, 9);

        // enable csi/mclk clock
        bk_pm_clock_ctrl(PM_CLK_ID_CSI, PM_CLK_CTRL_PWR_UP);
    }
    else
    {
        // disable csi mclk clock
        bk_pm_clock_ctrl(PM_CLK_ID_CSI, PM_CLK_CTRL_PWR_DOWN);
        #if CONFIG_USR_GPIO_CFG_EN
        gpio_dev_unmap_by_func(GPIO_DEV_JPEG_MCLK);
        #endif
    }
    return BK_OK;
}

static void isp_clock_enable(uint32_t clk)
{
    // isp pwd enable
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_ISP, PM_POWER_MODULE_STATE_ON);

    /* isp clock configuration 60MHz: sel 1, div 4 */
    sys_drv_cisp_cksel_clkdiv_set(CKSEL_CISP_240M, 1);
    // shared CISP clock vote (paired with bk_isp_clock_enable(0) in deinit)
    bk_isp_clock_enable(true);
}

static bk_err_t bk_isp_complete_buffer_config(isp_control_t *control, uint8_t chnl, uint8_t buf_cnt)
{
    bk_err_t ret = BK_FAIL;

    for (int i = 0; i < ISP_FRAME_CNT_MAX; i++) {
        VIDEO_BUF_S buf;
        uint32_t frame_size = 0;
        vsi_u8_t index;
        os_memset(&buf, 0, sizeof(buf));

        buf.index = i;
        buf.numPlanes = control->chn[chnl].chn_attr.chnFormat.numPlanes;
        for (index = 0; index < buf.numPlanes; index++)
        {
            buf.planes[index].size = control->chn[chnl].chn_attr.chnFormat.planeFmt[index].size;
            frame_size += buf.planes[index].size;
        }

        if (control->chn[chnl].frame_buffer[i] == NULL)
        {
#ifdef CONFIG_FRAME_BUFFER
            control->chn[chnl].frame_buffer[i] = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, frame_size);
#endif
            if (control->chn[chnl].frame_buffer[i] == NULL)
            {
                LOGE("%s, %d please attenation frame buffer not free\n", __func__, __LINE__);
                return ret;
            }
        }

        buf.planes[0].dmaPhyAddr = (vsi_dma_t)(uintptr_t)control->chn[chnl].frame_buffer[i];

        LOGI("stbuf[%d] fmt:%d \r\n", i, buf.numPlanes);
        LOGI("plane[%d]: addr %x, size %d \r\n", 0, buf.planes[0].dmaPhyAddr, buf.planes[0].size);

        for (index = 1; index < buf.numPlanes; index++) {
            buf.planes[index].dmaPhyAddr =
            buf.planes[index - 1].dmaPhyAddr +
            buf.planes[index - 1].size;

            LOGI("plane[%d]: addr %x, size %d \r\n", index, buf.planes[index].dmaPhyAddr, buf.planes[index].size);
        }

        VSI_MPI_ISP_QBUF(control->chn[chnl].channel, &buf);
    }

    ret = BK_OK;

    return ret;
}

static bk_err_t bk_isp_buffer_dequeue(ISP_CHN chnl, VIDEO_BUF_S *pBuf, uint32_t timeMs)
{
    vsi_u32_t timeout = timeMs;

    return VSI_MPI_ISP_DQBUF(chnl, pBuf, timeout);
}

static bk_err_t bk_isp_flexa_buffer_config(isp_control_t *control, uint8_t chnl, uint8_t buf_cnt)
{
    bk_err_t ret = BK_FAIL;

    FORMAT_S ring_format = {0};
    ring_format.width = control->chn[chnl].chn_attr.chnFormat.width;
    ring_format.height = control->chn[chnl].chn_attr.chnFormat.height;
    ring_format.pixelFormat = control->chn[chnl].chn_attr.chnFormat.pixelFormat;

    switch(ring_format.pixelFormat) {
        case PIXEL_FORMAT_NV16:
            break;

        case PIXEL_FORMAT_NV12:
            //mi format
            ring_format.numPlanes = 2;
            ring_format.imageSize = ring_format.width * ring_format.height;
            ring_format.planeFmt[0].bytesPerLine = ring_format.width;
            ring_format.planeFmt[0].size = ring_format.width * FLEXA_LINES * buf_cnt;
            ring_format.planeFmt[1].bytesPerLine = ring_format.width;
            ring_format.planeFmt[1].size = ring_format.width * FLEXA_LINES * buf_cnt / 2;
            //sbi config
            control->chn[chnl].sbi_attr.entryCnt = control->chn[chnl].buf_cnt;
            control->chn[chnl].sbi_attr.streamNum = 2; //y & uv
            control->chn[chnl].sbi_attr.streamAttr[0].entrySize = FLEXA_LINES;
            control->chn[chnl].sbi_attr.streamAttr[0].streamEnable = 1;
            control->chn[chnl].sbi_attr.streamAttr[1].entrySize = FLEXA_LINES / 2;
            control->chn[chnl].sbi_attr.streamAttr[1].streamEnable = 1;
            break;

        case PIXEL_FORMAT_YUV422P:
            break;

        case PIXEL_FORMAT_YUV420P:
            break;

        case PIXEL_FORMAT_YUYV:
            break;

        default:
            LOGE("Invalid pixel format %d\n", ring_format.pixelFormat);
            return ret;
    }

    ret = VSI_MPI_ISP_SetRingBufferFmt(control->chn[chnl].channel, &ring_format);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, ret:%d\n", __func__, __LINE__, ret);
        return ret;
    }

    VIDEO_BUF_S buf;
    uint32_t frame_size = 0;
    vsi_u8_t index;
    os_memset(&buf, 0, sizeof(buf));

    buf.index = 0;
    buf.numPlanes = ring_format.numPlanes;
    for (index = 0; index < buf.numPlanes; index++)
    {
        buf.planes[index].size = ring_format.planeFmt[index].size;
        frame_size += buf.planes[index].size;
    }

    if (control->chn[chnl].base_addr == NULL)
    {
        control->chn[chnl].base_addr = (uint8_t *)bk_get_isp_flexa_buffer(frame_size + 64);
        if (control->chn[chnl].base_addr == NULL)
        {
            LOGE("%s, %d malloc failed:%d\n", __func__, __LINE__, frame_size);
            ret = BK_FAIL;
            return ret;
        }
        control->chn[chnl].malloc_flag = true;
    }

    buf.planes[0].dmaPhyAddr = (((uint32_t)(uintptr_t)control->chn[chnl].base_addr + (64-1)) & ~(64-1));

    LOGD("buf_base:%p, numplanes:%d \r\n", control->chn[chnl].base_addr, buf.numPlanes);
    LOGD("plane[%d]: addr %x, size %d \r\n", 0, buf.planes[0].dmaPhyAddr, buf.planes[0].size);

    for (index = 1; index < buf.numPlanes; index++) {
        buf.planes[index].dmaPhyAddr =
        buf.planes[index - 1].dmaPhyAddr +
        buf.planes[index - 1].size;

        LOGI("plane[%d]: addr %x, size %d \r\n", index, buf.planes[index].dmaPhyAddr, buf.planes[index].size);
    }

    control->chn[chnl].y_addr = buf.planes[0].dmaPhyAddr;
    control->chn[chnl].u_addr = buf.planes[1].dmaPhyAddr;
    VSI_MPI_ISP_QBUF(control->chn[chnl].channel, &buf);

    return ret;
}

static bk_err_t bk_isp_device_config(isp_control_t *control)
{
    bk_err_t ret = BK_FAIL;

    control->dev = 0;
    control->port.devId = 0;
    control->port.portId = 0;
    VSI_MPI_ISP_Init(control->dev);

    /* Do NOT configure any port's ISP module pipeline here. The per-port
     * pipeline (which creates each module's mutex, e.g. WbV10 mLock) is set up
     * on demand when the upper layer brings a port up via bk_isp_port_init().
     * This keeps device config port-agnostic and only pays the cost for ports
     * that are actually used (e.g. ISP_DVP_PORT_ID only when DVP is opened). */

    ISP_DEV_ATTR_S devAttr;
    devAttr.ispWorkMode = WORK_MODE_NORMAL;
    ret = VSI_MPI_ISP_SetDevAttr(control->dev, &devAttr);
    if (ret)
    {
        LOGE("%s, VSI_MPI_ISP_SetDevAttr failed\n", __func__);
        return ret;
    }

    return ret;
}

bk_err_t bk_isp_port_init(isp_handle_t *handle, void *sensor_attr)
{
    bk_err_t ret = BK_FAIL;
    isp_control_t *control = (isp_control_t *)*handle;
    ISP_PUB_ATTR_S *pubAttr = (ISP_PUB_ATTR_S *)sensor_attr;
    if (pubAttr == NULL)
    {
        LOGE("%s, Not Support input:%d\n", __func__, __LINE__);
        return ret;
    }

    control->port.portId = pubAttr->port_id;
    control->pub_attr[control->port.portId] = pubAttr;

    /* Lazily initialise this port's ISP module pipeline the first time it is
     * brought up. VSI_MPI_ISP_PipeLineSet() creates the per-port module mutexes
     * (e.g. WbV10 mLock) that isp_set_port_attribute()->...->WbV10InitAlgo()
     * locks; without it the DVP port (ISP_DVP_PORT_ID) would lock a NULL mutex
     * and assert. The per-port guard makes it run exactly once per port so
     * repeated open/close (or MIPI<->DVP switching) won't re-create / leak it. */
    if (!(control->port_pipeline_inited & (1u << control->port.portId)))
    {
        VSI_MPI_ISP_PipeLineSet(control->port);
        control->port_pipeline_inited |= (1u << control->port.portId);
    }

    ret = isp_set_port_attribute(control->port, pubAttr);
    if (ret)
    {
        LOGE("%s, VSI_ISP_MPI_SetPubAttr failed\n", __func__);
        ret = BK_FAIL;;
    }
    else if (pubAttr->ispInputType == INPUT_TYPE_SENSOR)
    {
        control->sensor_sns_registered = 1;
    }

    return ret;
}

bk_err_t bk_isp_port_change(isp_handle_t *handle, uint8_t chnl)
{
    bk_err_t ret = BK_FAIL;
    isp_control_t *control = (isp_control_t *)*handle;
    control->port.portId = (control->port.portId + 1) % ISP_PORT_CNT;

    VSI_MPI_RESET(0, 0);
    VSI_MPI_RESET(0, 1);
    VSI_MPI_RESET(0, 2);
    VSI_MPI_RESET(0, 3);
    VSI_MPI_RESET(0, 6);

    if (control->port.portId == ISP_DVP_PORT_ID)
    {
        bk_mipi_csi_ext_set_enable(1);
    }
    else
    {
        bk_mipi_csi_ext_set_enable(0);
    }

    control->chn[chnl].channel.portId = control->port.portId;
    VSI_MPI_ISP_SetScaleAttr(control->chn[chnl].channel, &control->chn[chnl].chn_attr);
    
    ret = VSI_MPI_ISP_SetInput(control->port);
    VSI_MPI_RESET_CLEAR(0);

    return ret;
}

bk_err_t bk_isp_dev_init(isp_handle_t *handle)
{
    bk_err_t ret = BK_FAIL;
    if (*handle != NULL)
    {
        LOGE("%s, %p config error\n", __func__, *handle);
        return ret;
    }

    isp_control_t *isp_control = (isp_control_t *)os_malloc(sizeof(isp_control_t));
    if (isp_control == NULL)
    {
        LOGE("%s, %d malloc error\n", __func__, __LINE__);
        return ret;
    }
    os_memset(isp_control, 0, sizeof(isp_control_t));

    ret = rtos_init_semaphore(&isp_control->isp_sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s, %d init isp_sem\n", __func__, __LINE__);
        goto error;
    }

    // step 1: init isp clk
    isp_clock_enable(60000000);
    // step 2: init isp driver and input data config
    ret = bk_isp_device_config(isp_control);
    if (ret != BK_OK)
    {
        LOGE("%s, %d isp driver init fail\n", __func__, __LINE__);
        goto error;
    }

    // step 3: register isp_isr/mi_isr callback
    ISP_ISR_CBS_S cbs = {
        .isp_mis = isp_isr_callback_ext,
        .mi_mis = isp_mi_isr_callback_ext,
        .args = isp_control,
    };
    ret = VSI_MPI_ISP_RegIsrCallBack(isp_control->dev, cbs);
    if (ret != BK_OK)
    {
        LOGE("%s, %d regiister error!\n", __func__, __LINE__);
    }

    isp_control->pop_buf = bk_isp_buffer_dequeue;
    isp_control->free_buf = VSI_MPI_ISP_QBUF;
    isp_control->state = ISP_FSM_INIT;

    *handle = isp_control;

    if (ret != BK_OK)
    {
        LOGE("%s, %d malloc buffer error\n", __func__, __LINE__);
        goto error;
    }

    LOGI("%s, %d complete\n", __func__, __LINE__);

    return ret;

error:

    bk_isp_deinit((isp_handle_t)&isp_control);
    return ret;
}

bk_err_t bk_isp_deinit(isp_handle_t *handle)
{
    bk_err_t ret = BK_OK;
    if (*handle == NULL)
    {
        LOGW("%s, already deinit\n", __func__);
        return ret;
    }

    isp_control_t *control = (isp_control_t *)*handle;
    if (control->state != ISP_FSM_INIT)
    {
        LOGW("%s, can not in this state\n", __func__);
        ret = BK_FAIL;
        return ret;
    }

    if (control->isp_sem)
    {
        rtos_deinit_semaphore(&control->isp_sem);
    }

    for (uint8_t i = 0; i < ISP_CHN_MAX; i++)
    {
        if (control->chn[i].malloc_flag == true)
        {
            if (control->chn[i].base_addr)
            {
                os_free(control->chn[i].base_addr);
                control->chn[i].base_addr = NULL;
            }
        }
        else
        {
            control->chn[i].base_addr = NULL;
        }

        for (uint8_t j = 0; j < ISP_FRAME_CNT_MAX; j++)
        {
            if (control->chn[i].frame_buffer[j])
            {
                LOGI("%s, %d, freeing frame_buffer[%d][%d]: %p\n", __func__, __LINE__, i, j, control->chn[i].frame_buffer[j]);
#ifdef CONFIG_FRAME_BUFFER
                bk_frame_buffer_free(control->chn[i].frame_buffer[j]);
#endif
                control->chn[i].frame_buffer[j] = NULL;
            }
        }
    }

    VSI_MPI_ISP_DeRegIsrCallBack(control->dev);

    isp_unregister_sensor_callbacks(control);

    VSI_MPI_ISP_Exit(control->dev);

    // release shared CISP clock vote
    bk_isp_clock_enable(false);

    // disable isp pwd
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_ISP, PM_POWER_MODULE_STATE_OFF);

    os_free(control);
    *handle = NULL;

    //os_memset(&isp_isr_handler[0][0], 0, sizeof(isp_isr_handler_t) * ISP_ISR_MAX * ISP_ISR_MODULE_MAX);

    return ret;
}

bk_err_t bk_isp_open(isp_handle_t *handle, isp_config_ext_t *config)
{
    bk_err_t ret = BK_FAIL;

    if (*handle == NULL)
    {
        LOGE("%s, %d\n", __func__, __LINE__);
        return ret;
    }

    isp_control_t *control = (isp_control_t *)*handle;

    if (config->chnl_id >= ISP_CHN_MAX)
    {
        LOGE("%s, %d, chnl_id error\n", __func__, __LINE__);
        return ret;
    }

    if (control->chn[config->chnl_id].enable)
    {
        LOGE("%s, %d, this chnl already enable\n", __func__, __LINE__);
        return ret;
    }

    control->chn[config->chnl_id].channel.portId = config->port_id;

    // step 2: init isp chnel config
    control->chn[config->chnl_id].chn_attr.chnFormat.width = config->width;
    control->chn[config->chnl_id].chn_attr.chnFormat.height = config->height;
    control->chn[config->chnl_id].chn_attr.chnFormat.pixelFormat = config->format;
    control->chn[config->chnl_id].chn_attr.transBus = (config->buf_cnt == 0) ? TRANS_BUS_ONLINE : TRANS_BUS_FLEXA;
    if (config->work_mode == 0)
    {
        control->chn[config->chnl_id].chn_attr.transBus = TRANS_BUS_ONLINE;
    }
    control->chn[config->chnl_id].channel.chnId = config->chnl_id;
    control->chn[config->chnl_id].buf_cnt = config->buf_cnt;
    control->chn[config->chnl_id].skip_frames_remaining = config->skip_frames;
    control->chn[config->chnl_id].skip_active = 0;
    /* Inherit a peer that already finished warmup: AE is stable, skip nothing
     * and become an authority immediately. Otherwise only channels that run a
     * real (>0) skip countdown may later latch warmup_done in the ISR; a plain
     * skip==0 channel never becomes an authority so it can't cut a peer's
     * warmup short. */
    if (isp_peer_channel_warmup_done(control, config->chnl_id))
    {
        control->chn[config->chnl_id].skip_frames_remaining = 0;
        control->chn[config->chnl_id].warmup_done = 1;
    }
    else
    {
        control->chn[config->chnl_id].warmup_done = 0;
    }

    // step 3: config isp channel
    ret = VSI_MPI_ISP_SetChnAttr(control->chn[config->chnl_id].channel, &control->chn[config->chnl_id].chn_attr);
    if (ret != BK_OK) {
        LOGE("%s, %d, VSI_MPI_ISP_SetChnAttr failed, ret: %d\n", __func__, __LINE__, ret);
        return ret;
    }

    if (config->enable_flexa)
    {
        control->chn[config->chnl_id].enable_flexa = true;
        control->chn[config->chnl_id].total_line = (config->height % FLEXA_LINES) ? (config->height / FLEXA_LINES + 1) : (config->height / FLEXA_LINES);
        VSI_MPI_ISP_SetMiV10LineEnable(control->chn[config->chnl_id].channel, 1);
    }

    // step 4: frame config, maybe ringbuffer or frame_buffer
    if (config->work_mode == 0)
    {
        ret = bk_isp_complete_buffer_config(control, config->chnl_id, config->buf_cnt);
    }
    else
    {
        if (config->width == ISP_MAX_WIDTH)
        {
            control->chn[config->chnl_id].base_addr = bk_get_isp_flexa_buffer(config->buf_cnt * config->width * FLEXA_LINES * 3 / 2 + 64);
            if(control->chn[config->chnl_id].base_addr == NULL)
            {
                LOGE("%s, %d, bk_get_isp_flexa_buffer failed\n", __func__, __LINE__);
                return BK_FAIL;
            }
            control->chn[config->chnl_id].malloc_flag = true;
        }
        ret = bk_isp_flexa_buffer_config(control, config->chnl_id, config->buf_cnt);
    }

    if (ret != BK_OK)
    {
        LOGE("%s, %d buf malloc fail\n", __func__, __LINE__);
        return ret;
    }

    if (control->state == ISP_FSM_INIT)
    {

        ret = VSI_MPI_ISP_EnableDev(control->dev);
        if (ret != BK_OK)
        {
            LOGE("%s, %d, enable dev fail, ret=%d\n", __func__, __LINE__, ret);
        }

        ret = VSI_MPI_ISP_EnablePort(control->port);
        if (ret != BK_OK)
        {
            LOGE("%s, %d, enable port fail, ret=%d\n", __func__, __LINE__, ret);
        }

        control->state = ISP_FSM_CHN_ENABLE;
    }

    if (config->chnl_id == ISP_MP_CHN_ID)
    {
        AVDK_MONITOR_MP_ENABLE();
    }
    else
    {
        AVDK_MONITOR_SP_ENABLE();
    }

    ret = VSI_MPI_ISP_EnableChn(control->chn[config->chnl_id].channel);
    control->chn[config->chnl_id].enable = true;
    if (ret != BK_OK)
    {
        LOGE("%s, %d, enable chnl fail, ret=%d\n", __func__, __LINE__, ret);
        control->chn[config->chnl_id].enable = false;
    }

    return ret;
}

bk_err_t bk_isp_close(isp_handle_t *handle, uint8_t chnl)
{
    bk_err_t ret = BK_FAIL;
    uint8_t chnl_closed = true;

    if (*handle == NULL)
    {
        LOGE("%s, %d\n", __func__, __LINE__);
        return ret;
    }

    if (chnl > ISP_CHN_MAX)
    {
        LOGE("%s, %d, chnl error\n", __func__, __LINE__);
        return ret;
    }

    isp_control_t *control = (isp_control_t *)*handle;

    if (control->state != ISP_FSM_CHN_ENABLE)
    {
        LOGE("%s, %d, state error\n", __func__, __LINE__);
        return ret;
    }

    isp_channel_config_t *chnl_config = &control->chn[chnl];

    if (chnl_config->enable == false)
    {
        LOGI("%s, %d already close\n", __func__, __LINE__);
        return ret;
    }

    ret = VSI_MPI_ISP_DisableChn(control->chn[chnl].channel);
    chnl_config->enable = false;
    if (ret != BK_OK)
    {
        LOGE("%s, %d, disable chnl fail\n", __func__, __LINE__);
        chnl_config->enable = true;
    }

    for (uint8_t i = 0; i < ISP_FRAME_CNT_MAX; i++)
    {
        if (control->chn[i].enable)
        {
            chnl_closed = false;
            break;
        }
    }

    if (chnl_closed && control->state == ISP_FSM_CHN_ENABLE)
    {
        ret = VSI_MPI_ISP_DisablePort(control->port);
        if (ret != BK_OK)
        {
            LOGE("%s, %d, disable port fail, ret=%d\n", __func__, __LINE__, ret);
            return ret;
        }

        ret = VSI_MPI_ISP_DisableDev(control->dev);
        if (ret != BK_OK)
        {
            LOGE("%s, %d, disable dev fail, ret=%d\n", __func__, __LINE__, ret);
            return ret;
        }

        isp_unregister_sensor_callbacks(control);

        control->state = ISP_FSM_INIT;
    }

    return ret;
}

bk_err_t bk_isp_flexa_sbi_config(isp_handle_t *handle, uint8_t chnl, uint8_t enable)
{
    bk_err_t ret = BK_FAIL;

    if (*handle == NULL)
    {
        LOGE("%s, %d\n", __func__, __LINE__);
        return ret;
    }

    if (chnl >= ISP_CHN_MAX)
    {
        LOGE("%s, %d, chnl_id error\n", __func__, __LINE__);
        return ret;
    }

    isp_control_t *control = (isp_control_t *)*handle;
    VSI_FLEXA_SYNC_ATTR_S flexa_sync = {0};

    ISP_SBI_ATTR_S *isp_sbi_config = &control->chn[chnl].sbi_attr;

    if (enable)
    {
        if (isp_sbi_config->onLine)
        {
            return BK_OK;
        }

        isp_sbi_config->streamAttr[0].streamId = ISP_FLEXA_STREAM_ID_Y;
        isp_sbi_config->streamAttr[1].streamId = ISP_FLEXA_STREAM_ID_CB;
        isp_sbi_config->streamAttr[2].streamId = ISP_FLEXA_STREAM_ID_CR;
        isp_sbi_config->onLine = 1;

        switch(control->chn[chnl].chn_attr.chnFormat.pixelFormat) {
            case PIXEL_FORMAT_NV12:
                //sbi config
                isp_sbi_config->entryCnt = control->chn[chnl].buf_cnt;
                isp_sbi_config->streamNum = 2; //y & uv
                isp_sbi_config->streamAttr[0].entrySize = FLEXA_LINES;
                isp_sbi_config->streamAttr[0].streamEnable = 1;
                isp_sbi_config->streamAttr[1].entrySize = FLEXA_LINES / 2;
                isp_sbi_config->streamAttr[1].streamEnable = 1;
                break;

            default:
                LOGE("Invalid pixel format %d\n", control->chn[chnl].chn_attr.chnFormat.pixelFormat);
                return ret;
        }
    }
    else
    {
        if (!isp_sbi_config->onLine)
        {
            return BK_OK;
        }

        isp_sbi_config->onLine = 0;
    }

    flexa_sync.streamNum = isp_sbi_config->streamNum;
    flexa_sync.streamAttr[0].streamId = ISP_FLEXA_STREAM_ID_Y;
    flexa_sync.streamAttr[0].entryCnt = isp_sbi_config->streamAttr[0].entrySize;
    flexa_sync.streamAttr[1].streamId = ISP_FLEXA_STREAM_ID_CB;
    flexa_sync.streamAttr[1].entryCnt = isp_sbi_config->streamAttr[1].entrySize;
    flexa_sync.streamAttr[2].streamId = ISP_FLEXA_STREAM_ID_CR;
    flexa_sync.streamAttr[2].entryCnt = isp_sbi_config->streamAttr[2].entrySize;
    VSI_FLEXA_SetSyncAttr(control->chn[chnl].channel, &flexa_sync);
    ret = VSI_MPI_ISP_SetSbiProducer(control->chn[chnl].channel, isp_sbi_config);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, set sbi producer fail, %d\n", __func__, __LINE__, ret);
    }

    return ret;
}

bk_err_t bk_isp_register_isr_callback(isp_handle_t *handle, isp_isr_type_t type, isp_isr_t cb, void *arg)
{
    bk_err_t ret = BK_FAIL;
    uint8_t i = 0;
    int8_t registered_index = -1;
    bool already_registered = false;
    bool no_free_slot = false;

    if (*handle == NULL)
    {
        LOGE("%s, %d\n", __func__, __LINE__);
        return ret;
    }

    if (arg == NULL)
    {
        LOGE("%s, %d\n", __func__, __LINE__);
        return ret;
    }

    if (type >= ISP_ISR_MAX)
    {
        LOGE("%s, %d\n", __func__, __LINE__);
        return ret;
    }

    uint32_t irq_flags = isp_isr_lock_irqsave();

    for (i = 0; i < ISP_ISR_MODULE_MAX; i++)
    {
        if (isp_isr_handler[type][i].param == arg)
        {
            already_registered = true;
            break;
        }
    }

    if (i < ISP_ISR_MODULE_MAX)
    {
        isp_isr_unlock_irqrestore(irq_flags);
        goto error;
    }

    for (i = 0; i < ISP_ISR_MODULE_MAX; i++)
    {
        if (isp_isr_handler[type][i].param == NULL)
        {
            isp_isr_handler[type][i].isr_handler = cb;
            isp_isr_handler[type][i].param = arg;
            isp_isr_handler[type][i].enable = false;
            isp_isr_handler[type][i].reg_en = true;
            ret = BK_OK;
            registered_index = i;
            break;
        }
    }

    if (i == ISP_ISR_MODULE_MAX)
    {
        no_free_slot = true;
    }

    isp_isr_unlock_irqrestore(irq_flags);

error:
    if (already_registered)
    {
        LOGW("%s, %d already register\n", __func__, __LINE__);
    }
    else if (no_free_slot)
    {
        LOGW("%s, %d over invalid range\n", __func__, __LINE__);
    }
    else if (registered_index >= 0)
    {
        LOGI("%s, %d, register isr callback %d success\n", __func__, __LINE__, registered_index);
    }
    LOGI("%s, %d, %d\n", __func__, __LINE__, type);

    return ret;
}

bk_err_t bk_isp_deregister_isr_callback(isp_handle_t *handle, isp_isr_type_t type, void *arg)
{
    bk_err_t ret = BK_FAIL;
    uint8_t i = 0;

    if (*handle == NULL)
    {
        LOGE("%s, %d\n", __func__, __LINE__);
        return ret;
    }

    uint32_t irq_flags = isp_isr_lock_irqsave();

    for (i = 0; i < ISP_ISR_MODULE_MAX; i++)
    {
        if (isp_isr_handler[type][i].param == arg)
        {
            isp_isr_handler[type][i].reg_en = false;
            os_memset(&isp_isr_handler[type][i], 0, sizeof(isp_isr_handler_t));
            ret = BK_OK;
            break;
        }
    }

    isp_isr_unlock_irqrestore(irq_flags);

    return ret;
}

bk_err_t bk_isp_soft_reset(isp_handle_t *handle)
{
    bk_err_t ret = BK_FAIL;

    if (*handle == NULL)
    {
        LOGE("%s, %d\n", __func__, __LINE__);
        return ret;
    }

    isp_control_t *control = (isp_control_t *)*handle;

    // isp soft reset
    VSI_MPI_RESET(control->dev, 0);
    // color process soft reset
    VSI_MPI_RESET(control->dev, 1);
    // y/c split process soft reset
    VSI_MPI_RESET(control->dev, 2);
    // Main-picture resize soft reset
    VSI_MPI_RESET(control->dev, 3);
    // memory interface soft reset
    VSI_MPI_RESET(control->dev, 6);

    // restart isp enable
    VSI_MPI_RESET_CLEAR(control->dev);

    return BK_OK;
}

bk_err_t bk_isp_get_exposure_luminance(isp_handle_t *handle, uint32_t *luminance)
{
    if (handle == NULL || *handle == NULL || luminance == NULL)
    {
        LOGE("%s, invalid parameter\n", __func__);
        return BK_FAIL;
    }

    isp_control_t *control = (isp_control_t *)*handle;
    ISP_EXPOSURE_INFO_S exposure_info = {0};
    int ret = VSI_MPI_ISP_QueryExposureInfo(control->port, &exposure_info);
    if (ret != VSI_SUCCESS)
    {
        LOGE("%s, query exposure info failed: %d\n", __func__, ret);
        return BK_FAIL;
    }

    *luminance = exposure_info.meanLum;
    return BK_OK;
}