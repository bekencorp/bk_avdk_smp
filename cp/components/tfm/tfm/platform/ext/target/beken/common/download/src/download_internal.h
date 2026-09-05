// Copyright 2026 Beken
//
// Internal declarations shared by the BL2 download implementation.

#pragma once

#include <stdint.h>
#include "type.h"
#include "download_boot.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32_t baud_rate;
	uint8_t rx_fifo_threshold;
	uint8_t tx_fifo_threshold;
} download_uart_config_t;

#define RX_CMD_BUFF_SIZE        (4200)
#define TBL_SIZE(tbl)           (sizeof(tbl) / sizeof((tbl)[0]))
#define DOWNLOAD_WDT_VALUE      (0xFFFF)

enum {
	CMD_TYPE_INVALID = 0,
	CMD_TYPE_COMMON,
	CMD_TYPE_FLASH,
};

typedef struct {
	u8 frm_type;
	u8 frm_done;
	u8 hci_hdr_state;
	u8 hci_rx_state;
	u8 cmd_type;
	u16 cmd_len;
	u16 cmd_id;
	u32 cmd_param[RX_CMD_BUFF_SIZE / sizeof(u32)];
	u16 write_idx;
	u16 read_idx;
	u8 status;
} rx_frm_ctrl_t;

extern u8 uart_link_check_flag;
extern u32 download_record_dl_flag;

void download_uart_init(const download_uart_config_t *cfg);
void download_uart_set_baudrate(uint32_t baud_rate);
void download_uart_disable(void);
void download_uart_write(const uint8_t *buf, uint32_t len);

void wdt_time_set(uint32_t val);
uint32_t timer_init(uint32_t us);
void timer_delay_ms(uint32_t ms);
void timer_delay_us(uint32_t us);

void tx_rsp_data(u8 *buf, u16 len);
void tx_rsp_for_common_cmd(u16 cmd_id, u8 *cmd_param, u16 param_len);
void tx_rsp_for_flash_cmd_hdr(u16 cmd_id, u8 status, u16 param_len);
void tx_rsp_for_flash_cmd(u16 cmd_id, u8 status, u8 *cmd_param, u16 param_len);

u32 common_cmd_process(rx_frm_ctrl_t *frm_ctrl);
u32 flash_cmd_process(rx_frm_ctrl_t *frm_ctrl);
u32 flash_cmd_sector_write(rx_frm_ctrl_t *frm_ctrl);
u32 flash_cmd_sector_write_done(rx_frm_ctrl_t *frm_ctrl);

#ifdef __cplusplus
}
#endif
