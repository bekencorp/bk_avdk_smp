#ifndef __DMA2D_TEST_H__
#define __DMA2D_TEST_H__

#include <os/os.h>
#include "components/bk_dma2d.h"

#ifdef __cplusplus
extern "C" {
#endif
int cli_dma2d_init(void);
/* 辅助函数声明 */
avdk_err_t memcopy_test_check(uint16_t *dst, uint16_t *src, uint8_t pixel_byte, uint32_t width, uint32_t height);

/* 测试用例函数声明 */
int dma2d_fill_test(bk_dma2d_ctlr_handle_t handle, const char *format, uint32_t color,
                   uint16_t frame_width, uint16_t frame_height,
                   uint16_t xpos, uint16_t ypos,
                   uint16_t fill_width, uint16_t fill_height);

int dma2d_memcpy_test(bk_dma2d_ctlr_handle_t handle, const char *format, uint32_t color,
                     uint16_t src_width, uint16_t src_height,
                     uint16_t dst_width, uint16_t dst_height,
                     uint16_t src_frame_xpos, uint16_t src_frame_ypos,
                     uint16_t dst_frame_xpos, uint16_t dst_frame_ypos,
                     uint16_t dma2d_width, uint16_t dma2d_height);

int dma2d_pfc_test(bk_dma2d_ctlr_handle_t handle, const char *input_format, const char *output_format,
                  uint32_t color, uint16_t src_width, uint16_t src_height,
                  uint16_t dst_width, uint16_t dst_height,
                  uint16_t src_frame_xpos, uint16_t src_frame_ypos,
                  uint16_t dst_frame_xpos, uint16_t dst_frame_ypos,
                  uint16_t dma2d_width, uint16_t dma2d_height);

int dma2d_blend_test(bk_dma2d_ctlr_handle_t handle, const char *bg_format, uint32_t bg_color,
                    uint16_t bg_width, uint16_t bg_height, bool is_sync);

                    
#ifdef __cplusplus
}
#endif

#endif /* __DMA2D_TEST_H__ */