/*
 * Copyright (c) 2025 BekenCorp. All rights reserved.
 * 
 * This software is proprietary and confidential. No part of this software may be
 * reproduced, distributed, or transmitted in any form or by any means, including
 * photocopying, recording, or other electronic or mechanical methods, without the
 * prior written permission of BekenCorp, except in the case of brief quotations
 * embodied in critical reviews and certain other noncommercial uses permitted
 * by copyright law.
 * 
 * For permission requests, write to BekenCorp at armino_support@bekencorp.com.

 * Author: Beken LVGL Designer Tool
*/ 
#include "lvgl.h"
#include "beken_ui.h"
#include "custom_func.h"
#include "event_runtime.h"
#include <stdio.h>
#include <string.h>

// please implement weak symbol function in custom_func.h custom_func.c
void __attribute__((weak)) key1_clicked(lv_event_t *e) {}

// please implement weak symbol function in custom_func.h custom_func.c
void __attribute__((weak)) key2_clicked(lv_event_t *e) {}

// please implement weak symbol function in custom_func.h custom_func.c
void __attribute__((weak)) key3_clicked(lv_event_t *e) {}

/**
 * @brief Event callback for page_1_button_1 - handles all events
 * @param e LVGL event object
 */
void page_1_button_1_event_cb(lv_event_t *e)
{
    // bk_lv_ui_t *bk_ui = &bk_lv_tool_ui;
    // lv_event_code_t code = lv_event_get_code(e);
    // lv_obj_t * target = lv_event_get_target(e);
    // if (code == LV_EVENT_CLICKED) {
    //     key1_clicked(e);
    // }
}

/**
 * @brief Event callback for page_1_button_2 - handles all events
 * @param e LVGL event object
 */
void page_1_button_2_event_cb(lv_event_t *e)
{
    // bk_lv_ui_t *bk_ui = &bk_lv_tool_ui;
    // lv_event_code_t code = lv_event_get_code(e);
    // lv_obj_t * target = lv_event_get_target(e);
    // if (code == LV_EVENT_CLICKED) {
    //     key2_clicked(e);
    // }
}

/**
 * @brief Event callback for page_1_button_3 - handles all events
 * @param e LVGL event object
 */
void page_1_button_3_event_cb(lv_event_t *e)
{
    // bk_lv_ui_t *bk_ui = &bk_lv_tool_ui;
    // lv_event_code_t code = lv_event_get_code(e);
    // lv_obj_t * target = lv_event_get_target(e);
    // if (code == LV_EVENT_CLICKED) {
    //     key3_clicked(e);
    // }
}

/*
 * @brief: init page page_1
 */
void init_page_page_1(bk_lv_ui_t *bk_ui)
{
    if (bk_ui->page_1 != NULL && lv_obj_is_valid(bk_ui->page_1)) {
        destroy_page_page_1(bk_ui);
    }


    bk_ui->page_1 = lv_obj_create(NULL);
    lv_obj_set_scrollbar_mode(bk_ui->page_1, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(bk_ui->page_1, 1088, 832);
    lv_obj_set_style_bg_color(bk_ui->page_1, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bk_ui->page_1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(bk_ui->page_1, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);

    bk_ui->page_1_image_1 = lv_image_create(bk_ui->page_1);
    lv_image_set_pivot(bk_ui->page_1_image_1, 50, 50);
    lv_image_set_rotation(bk_ui->page_1_image_1, 0);
    lv_obj_set_x(bk_ui->page_1_image_1, 70);
    lv_obj_set_y(bk_ui->page_1_image_1, 58);
    lv_obj_set_width(bk_ui->page_1_image_1, 260);
    lv_obj_set_height(bk_ui->page_1_image_1, 300);
    lv_obj_set_style_bg_color(bk_ui->page_1_image_1, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bk_ui->page_1_image_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(bk_ui->page_1_image_1, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(bk_ui->page_1_image_1, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(bk_ui->page_1_image_1, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(bk_ui->page_1_image_1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(bk_ui->page_1_image_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bk_ui->page_1_image_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(bk_ui->page_1_image_1, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(bk_ui->page_1_image_1, lv_color_hex(0x1e7fcf), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(bk_ui->page_1_image_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(bk_ui->page_1_image_1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_x(bk_ui->page_1_image_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_y(bk_ui->page_1_image_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(bk_ui->page_1_image_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(bk_ui->page_1_image_1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_image_recolor(bk_ui->page_1_image_1, lv_color_hex(0x00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_image_recolor_opa(bk_ui->page_1_image_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    bk_ui->page_1_image_2 = lv_image_create(bk_ui->page_1);
    lv_image_set_pivot(bk_ui->page_1_image_2, 50, 50);
    lv_image_set_rotation(bk_ui->page_1_image_2, 0);
    lv_obj_set_x(bk_ui->page_1_image_2, 412);
    lv_obj_set_y(bk_ui->page_1_image_2, 58);
    lv_obj_set_width(bk_ui->page_1_image_2, 260);
    lv_obj_set_height(bk_ui->page_1_image_2, 300);
    lv_obj_set_style_bg_color(bk_ui->page_1_image_2, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bk_ui->page_1_image_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(bk_ui->page_1_image_2, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(bk_ui->page_1_image_2, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(bk_ui->page_1_image_2, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(bk_ui->page_1_image_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(bk_ui->page_1_image_2, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bk_ui->page_1_image_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(bk_ui->page_1_image_2, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(bk_ui->page_1_image_2, lv_color_hex(0x1e7fcf), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(bk_ui->page_1_image_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(bk_ui->page_1_image_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_x(bk_ui->page_1_image_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_y(bk_ui->page_1_image_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(bk_ui->page_1_image_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(bk_ui->page_1_image_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_image_recolor(bk_ui->page_1_image_2, lv_color_hex(0x00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_image_recolor_opa(bk_ui->page_1_image_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    bk_ui->page_1_image_3 = lv_image_create(bk_ui->page_1);
    lv_image_set_pivot(bk_ui->page_1_image_3, 50, 50);
    lv_image_set_rotation(bk_ui->page_1_image_3, 0);
    lv_obj_set_x(bk_ui->page_1_image_3, 749);
    lv_obj_set_y(bk_ui->page_1_image_3, 58);
    lv_obj_set_width(bk_ui->page_1_image_3, 260);
    lv_obj_set_height(bk_ui->page_1_image_3, 300);
    lv_obj_set_style_bg_color(bk_ui->page_1_image_3, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bk_ui->page_1_image_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(bk_ui->page_1_image_3, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(bk_ui->page_1_image_3, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(bk_ui->page_1_image_3, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(bk_ui->page_1_image_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(bk_ui->page_1_image_3, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bk_ui->page_1_image_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(bk_ui->page_1_image_3, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(bk_ui->page_1_image_3, lv_color_hex(0x1e7fcf), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(bk_ui->page_1_image_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(bk_ui->page_1_image_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_x(bk_ui->page_1_image_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_y(bk_ui->page_1_image_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(bk_ui->page_1_image_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_image_opa(bk_ui->page_1_image_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_image_recolor(bk_ui->page_1_image_3, lv_color_hex(0x00ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_image_recolor_opa(bk_ui->page_1_image_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    bk_ui->page_1_textarea_2 = lv_textarea_create(bk_ui->page_1);
    lv_obj_set_x(bk_ui->page_1_textarea_2, 412);
    lv_obj_set_y(bk_ui->page_1_textarea_2, 388);
    lv_obj_set_width(bk_ui->page_1_textarea_2, 260);
    lv_obj_set_height(bk_ui->page_1_textarea_2, 80);
    lv_textarea_set_text(bk_ui->page_1_textarea_2, "UNKNOWN");
    lv_textarea_set_placeholder_text(bk_ui->page_1_textarea_2, "");
    lv_textarea_set_password_mode(bk_ui->page_1_textarea_2, false);
    lv_textarea_set_one_line(bk_ui->page_1_textarea_2, false);
    lv_textarea_set_accepted_chars(bk_ui->page_1_textarea_2, "");
    lv_obj_set_style_bg_color(bk_ui->page_1_textarea_2, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bk_ui->page_1_textarea_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(bk_ui->page_1_textarea_2, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(bk_ui->page_1_textarea_2, lv_color_hex(0xdbdfea), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(bk_ui->page_1_textarea_2, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(bk_ui->page_1_textarea_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(bk_ui->page_1_textarea_2, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bk_ui->page_1_textarea_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(bk_ui->page_1_textarea_2, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(bk_ui->page_1_textarea_2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(bk_ui->page_1_textarea_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(bk_ui->page_1_textarea_2, &lv_font_montserrat_regular_36, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(bk_ui->page_1_textarea_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(bk_ui->page_1_textarea_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(bk_ui->page_1_textarea_2, lv_color_hex(0x1e7fcf), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(bk_ui->page_1_textarea_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(bk_ui->page_1_textarea_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_x(bk_ui->page_1_textarea_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_y(bk_ui->page_1_textarea_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(bk_ui->page_1_textarea_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    bk_ui->page_1_textarea_1 = lv_textarea_create(bk_ui->page_1);
    lv_obj_set_x(bk_ui->page_1_textarea_1, 70);
    lv_obj_set_y(bk_ui->page_1_textarea_1, 388);
    lv_obj_set_width(bk_ui->page_1_textarea_1, 260);
    lv_obj_set_height(bk_ui->page_1_textarea_1, 80);
    lv_textarea_set_text(bk_ui->page_1_textarea_1, "UNKNOWN");
    lv_textarea_set_placeholder_text(bk_ui->page_1_textarea_1, "");
    lv_textarea_set_password_mode(bk_ui->page_1_textarea_1, false);
    lv_textarea_set_one_line(bk_ui->page_1_textarea_1, false);
    lv_textarea_set_accepted_chars(bk_ui->page_1_textarea_1, "");
    lv_obj_set_style_bg_color(bk_ui->page_1_textarea_1, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bk_ui->page_1_textarea_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(bk_ui->page_1_textarea_1, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(bk_ui->page_1_textarea_1, lv_color_hex(0xdbdfea), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(bk_ui->page_1_textarea_1, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(bk_ui->page_1_textarea_1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(bk_ui->page_1_textarea_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bk_ui->page_1_textarea_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(bk_ui->page_1_textarea_1, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(bk_ui->page_1_textarea_1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(bk_ui->page_1_textarea_1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(bk_ui->page_1_textarea_1, &lv_font_montserrat_regular_36, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(bk_ui->page_1_textarea_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(bk_ui->page_1_textarea_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(bk_ui->page_1_textarea_1, lv_color_hex(0x1e7fcf), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(bk_ui->page_1_textarea_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(bk_ui->page_1_textarea_1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_x(bk_ui->page_1_textarea_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_y(bk_ui->page_1_textarea_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(bk_ui->page_1_textarea_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    bk_ui->page_1_textarea_3 = lv_textarea_create(bk_ui->page_1);
    lv_obj_set_x(bk_ui->page_1_textarea_3, 749);
    lv_obj_set_y(bk_ui->page_1_textarea_3, 388);
    lv_obj_set_width(bk_ui->page_1_textarea_3, 260);
    lv_obj_set_height(bk_ui->page_1_textarea_3, 80);
    lv_textarea_set_text(bk_ui->page_1_textarea_3, "UNKNOWN");
    lv_textarea_set_placeholder_text(bk_ui->page_1_textarea_3, "");
    lv_textarea_set_password_mode(bk_ui->page_1_textarea_3, false);
    lv_textarea_set_one_line(bk_ui->page_1_textarea_3, false);
    lv_textarea_set_accepted_chars(bk_ui->page_1_textarea_3, "");
    lv_obj_set_style_bg_color(bk_ui->page_1_textarea_3, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bk_ui->page_1_textarea_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(bk_ui->page_1_textarea_3, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(bk_ui->page_1_textarea_3, lv_color_hex(0xdbdfea), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(bk_ui->page_1_textarea_3, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(bk_ui->page_1_textarea_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(bk_ui->page_1_textarea_3, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bk_ui->page_1_textarea_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(bk_ui->page_1_textarea_3, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(bk_ui->page_1_textarea_3, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(bk_ui->page_1_textarea_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(bk_ui->page_1_textarea_3, &lv_font_montserrat_regular_36, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(bk_ui->page_1_textarea_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(bk_ui->page_1_textarea_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(bk_ui->page_1_textarea_3, lv_color_hex(0x1e7fcf), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(bk_ui->page_1_textarea_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(bk_ui->page_1_textarea_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_x(bk_ui->page_1_textarea_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_y(bk_ui->page_1_textarea_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(bk_ui->page_1_textarea_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    bk_ui->page_1_button_1 = lv_btn_create(bk_ui->page_1);
    bk_ui->page_1_button_1_label = lv_label_create(bk_ui->page_1_button_1);
    lv_label_set_text(bk_ui->page_1_button_1_label, "KEY 1");
    lv_label_set_long_mode(bk_ui->page_1_button_1_label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_align(bk_ui->page_1_button_1_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_x(bk_ui->page_1_button_1, 70);
    lv_obj_set_y(bk_ui->page_1_button_1, 494);
    lv_obj_set_width(bk_ui->page_1_button_1, 260);
    lv_obj_set_height(bk_ui->page_1_button_1, 100);
    lv_obj_set_style_bg_color(bk_ui->page_1_button_1, lv_color_hex(0x2d75b9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bk_ui->page_1_button_1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(bk_ui->page_1_button_1, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(bk_ui->page_1_button_1, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(bk_ui->page_1_button_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(bk_ui->page_1_button_1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(bk_ui->page_1_button_1, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bk_ui->page_1_button_1, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(bk_ui->page_1_button_1, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(bk_ui->page_1_button_1, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(bk_ui->page_1_button_1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(bk_ui->page_1_button_1, &lv_font_montserrat_regular_36, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(bk_ui->page_1_button_1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(bk_ui->page_1_button_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(bk_ui->page_1_button_1, lv_color_hex(0x1e7fcf), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(bk_ui->page_1_button_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(bk_ui->page_1_button_1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_x(bk_ui->page_1_button_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_y(bk_ui->page_1_button_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(bk_ui->page_1_button_1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(bk_ui->page_1_button_1, page_1_button_1_event_cb, LV_EVENT_ALL, NULL);

    bk_ui->page_1_button_2 = lv_btn_create(bk_ui->page_1);
    bk_ui->page_1_button_2_label = lv_label_create(bk_ui->page_1_button_2);
    lv_label_set_text(bk_ui->page_1_button_2_label, "KEY 2");
    lv_label_set_long_mode(bk_ui->page_1_button_2_label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_align(bk_ui->page_1_button_2_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_x(bk_ui->page_1_button_2, 412);
    lv_obj_set_y(bk_ui->page_1_button_2, 492);
    lv_obj_set_width(bk_ui->page_1_button_2, 260);
    lv_obj_set_height(bk_ui->page_1_button_2, 100);
    lv_obj_set_style_bg_color(bk_ui->page_1_button_2, lv_color_hex(0x2d75b9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bk_ui->page_1_button_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(bk_ui->page_1_button_2, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(bk_ui->page_1_button_2, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(bk_ui->page_1_button_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(bk_ui->page_1_button_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(bk_ui->page_1_button_2, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bk_ui->page_1_button_2, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(bk_ui->page_1_button_2, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(bk_ui->page_1_button_2, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(bk_ui->page_1_button_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(bk_ui->page_1_button_2, &lv_font_montserrat_regular_36, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(bk_ui->page_1_button_2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(bk_ui->page_1_button_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(bk_ui->page_1_button_2, lv_color_hex(0x1e7fcf), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(bk_ui->page_1_button_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(bk_ui->page_1_button_2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_x(bk_ui->page_1_button_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_y(bk_ui->page_1_button_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(bk_ui->page_1_button_2, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(bk_ui->page_1_button_2, page_1_button_2_event_cb, LV_EVENT_ALL, NULL);

    bk_ui->page_1_button_3 = lv_btn_create(bk_ui->page_1);
    bk_ui->page_1_button_3_label = lv_label_create(bk_ui->page_1_button_3);
    lv_label_set_text(bk_ui->page_1_button_3_label, "KEY 3");
    lv_label_set_long_mode(bk_ui->page_1_button_3_label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_align(bk_ui->page_1_button_3_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_x(bk_ui->page_1_button_3, 749);
    lv_obj_set_y(bk_ui->page_1_button_3, 492);
    lv_obj_set_width(bk_ui->page_1_button_3, 260);
    lv_obj_set_height(bk_ui->page_1_button_3, 100);
    lv_obj_set_style_bg_color(bk_ui->page_1_button_3, lv_color_hex(0x2d75b9), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bk_ui->page_1_button_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(bk_ui->page_1_button_3, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(bk_ui->page_1_button_3, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(bk_ui->page_1_button_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(bk_ui->page_1_button_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(bk_ui->page_1_button_3, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bk_ui->page_1_button_3, 5, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(bk_ui->page_1_button_3, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(bk_ui->page_1_button_3, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(bk_ui->page_1_button_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(bk_ui->page_1_button_3, &lv_font_montserrat_regular_36, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(bk_ui->page_1_button_3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(bk_ui->page_1_button_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(bk_ui->page_1_button_3, lv_color_hex(0x1e7fcf), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(bk_ui->page_1_button_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(bk_ui->page_1_button_3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_x(bk_ui->page_1_button_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_y(bk_ui->page_1_button_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(bk_ui->page_1_button_3, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(bk_ui->page_1_button_3, page_1_button_3_event_cb, LV_EVENT_ALL, NULL);

    bk_ui->page_1_textarea_5 = lv_textarea_create(bk_ui->page_1);
    lv_obj_set_x(bk_ui->page_1_textarea_5, 71);
    lv_obj_set_y(bk_ui->page_1_textarea_5, 611);
    lv_obj_set_width(bk_ui->page_1_textarea_5, 941);
    lv_obj_set_height(bk_ui->page_1_textarea_5, 195);
    lv_textarea_set_text(bk_ui->page_1_textarea_5, "AI:");
    lv_textarea_set_placeholder_text(bk_ui->page_1_textarea_5, "");
    lv_textarea_set_password_mode(bk_ui->page_1_textarea_5, false);
    lv_textarea_set_one_line(bk_ui->page_1_textarea_5, false);
    lv_textarea_set_accepted_chars(bk_ui->page_1_textarea_5, "");
    lv_obj_set_style_bg_color(bk_ui->page_1_textarea_5, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(bk_ui->page_1_textarea_5, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(bk_ui->page_1_textarea_5, LV_GRAD_DIR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(bk_ui->page_1_textarea_5, lv_color_hex(0xdbdfea), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(bk_ui->page_1_textarea_5, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(bk_ui->page_1_textarea_5, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_side(bk_ui->page_1_textarea_5, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bk_ui->page_1_textarea_5, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(bk_ui->page_1_textarea_5, false, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(bk_ui->page_1_textarea_5, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(bk_ui->page_1_textarea_5, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(bk_ui->page_1_textarea_5, &lv_font_montserrat_regular_36, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(bk_ui->page_1_textarea_5, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_letter_space(bk_ui->page_1_textarea_5, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(bk_ui->page_1_textarea_5, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(bk_ui->page_1_textarea_5, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(bk_ui->page_1_textarea_5, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_x(bk_ui->page_1_textarea_5, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_offset_y(bk_ui->page_1_textarea_5, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_spread(bk_ui->page_1_textarea_5, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_update_layout(bk_ui->page_1);
}

/*
 * @brief: destroy page page_1
 */
void destroy_page_page_1(bk_lv_ui_t *bk_ui)
{
    if (bk_ui == NULL) {
        return;
    }

    if (bk_ui->page_1 != NULL) {
        lv_obj_del(bk_ui->page_1);
        bk_ui->page_1 = NULL;
    }
}