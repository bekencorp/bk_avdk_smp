
#pragma once

#ifdef __cplusplus
extern "C" {
#endif


bk_err_t bk_lcd_panel_io_rx_param(bk_lcd_bus_io_t * io, int lcd_cmd, void *param, size_t param_size);

bk_err_t bk_lcd_panel_io_tx_param(bk_lcd_bus_io_t * io, int lcd_cmd, const void *param, size_t param_size);

bk_err_t bk_lcd_panel_io_del(bk_lcd_bus_io_t * io);


/*
 * @}
 */

#ifdef __cplusplus
}
#endif


