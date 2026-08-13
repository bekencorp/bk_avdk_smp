#ifndef _DPU_DRIVER_H_
#define _DPU_DRIVER_H_

#include <stdbool.h>
#include <stdint.h>
#include "soc/bk7259/reg_base.h"

#define FPGA_7259_A35       1
#define FPGA_7259_CM55      2
#define SOC_7259_A35        3
#define SOC_7259_CM55       4
#define SFT_VERSION         SOC_7259_A35

#define BK7259_DPU_REG_BASE_ADDR  SOC_DPU_REG_BASE

#define PSRAM_BUFFER_ENABLE 1 

#if PSRAM_BUFFER_ENABLE
#define LAYER_BUFFER0 0x64100000
#define LAYER_BUFFER1 0x64500000
#define LAYER_BUFFER2 0x64900000
#endif

#define LAYER_NUM     2
#define DISPLAY_NUM   1

typedef struct _layer_config
{
   /* layer */
    bool layer_enable;

    /* layer buffer format*/
    uint16_t format;

    /* decompress */
    bool decompress_enable;

    /* alpha blend */
    bool blend_enable;
    uint8_t blend_mode;

    /* layer resolution ratio */
    uint16_t width;
    uint16_t height;

    /* display rectangle */
    uint32_t   disp_x;         /* Rectangle start point X coordinate */
    uint32_t   disp_y;         /* Rectangle start point Y coordinate */
    uint32_t   disp_w;         /* Rectangle width*/
    uint32_t   disp_h;         /* Rectangle height */
}layer_config;

/* DPI parallel-RGB wire OUTPUT coding for dpu_frame_set_dpi_out_format(). Maps
 * 1:1 onto the DPI_CONFIG.OUTPUT_FORMAT register field. */
typedef enum {
    DPU_DPI_OUT_RGB565_CFG1 = 0,   /**< D16CFG1 */
    DPU_DPI_OUT_RGB565_CFG2,       /**< D16CFG2 */
    DPU_DPI_OUT_RGB565_CFG3,       /**< D16CFG3 */
    DPU_DPI_OUT_RGB666_CFG1,       /**< D18CFG1 */
    DPU_DPI_OUT_RGB666_CFG2,       /**< D18CFG2 */
    DPU_DPI_OUT_RGB888,            /**< D24 */
} dpu_dpi_out_format_t;

typedef int (*dpu_isr_cb_t)(void *params);               /**< jpegdec int isr register func type */

void dpu_isr(void);
int dpu_frame_init(void);
int dpu_frame_deinit(void);
int dpu_frame_layer_config(layer_config layers[LAYER_NUM]);
int dpu_frame_display_config( uint16_t width,
                              uint16_t height,
                              uint8_t  hsync_pulse_width,
                              uint16_t hsync_back_porch,
                              uint16_t hsync_front_porch,
                              uint8_t  vsync_pulse_width,
                              uint16_t vsync_back_porch,
                              uint16_t vsync_front_porch
                              );
int dpu_frame_commit(uint8_t layer_id, uint8_t *buff);
int dpu_frame_update(uint8_t layer_id, uint8_t *buff);
int dpu_frame_switch_video_config(layer_config *layer);
int dpu_frame_flush_isr(uint32_t *vblank_count);
int dpu_frame_flush_complete_register(dpu_isr_cb_t dpu_cb, void *cb_data);
int dpu_frame_get_layer_address(uint8_t layer_id);
int dpu_frame_trigger(uint8_t value);

/* Static DPI-side output coding. Updates the display output-format truth-source
 * and re-pushes the display config (marking it dirty); the caller triggers a
 * commit so it latches at the next frame boundary. Meant to be called once
 * around open. Returns 0 (vivSTATUS_OK) on success. */
int dpu_frame_set_dpi_out_format(dpu_dpi_out_format_t dpi_out_format);

#endif
