#ifndef __DRAW_OSD_COMPLEX_TEST_H__
#define __DRAW_OSD_COMPLEX_TEST_H__

#include <components/avdk_utils/avdk_error.h>
#include "components/bk_draw_osd.h"
#include "components/bk_display.h"
#include "osd_disp_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 由 draw_osd_cli.c 持有的全局句柄
 * 复杂测试用例复用，避免多份初始化逻辑
 * 前置：osd init 必须已经成功执行
 */
extern bk_draw_osd_ctlr_handle_t draw_osd_handle;
extern bk_display_ctlr_handle_t  lcd_display_handle;

/*
 * CLI 子命令入口：osd test <sub> [arg]
 * 支持：stability / concurrent / multi_inst / invalid_param /
 *       cfg_unchanged / shrink / unaligned / all
 */
avdk_err_t osd_complex_test_dispatch(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif /* __DRAW_OSD_COMPLEX_TEST_H__ */
