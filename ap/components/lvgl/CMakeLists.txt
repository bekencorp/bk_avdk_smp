armino_build_get_property(project_dir PROJECT_DIR)

if(CONFIG_LVGL)
	if(CONFIG_LVGL_V8)
		set(incs
			lvgl_v8/.
			lvgl_v8/porting
		)

		set(srcs
			lvgl_v8/src/core/lv_disp.c
			lvgl_v8/src/core/lv_group.c
			lvgl_v8/src/core/lv_indev.c
			lvgl_v8/src/core/lv_indev_scroll.c
			lvgl_v8/src/core/lv_obj.c
			lvgl_v8/src/core/lv_obj_class.c
			lvgl_v8/src/core/lv_obj_draw.c
			lvgl_v8/src/core/lv_obj_pos.c
			lvgl_v8/src/core/lv_obj_scroll.c
			lvgl_v8/src/core/lv_obj_style.c
			lvgl_v8/src/core/lv_obj_style_gen.c
			lvgl_v8/src/core/lv_obj_tree.c
			lvgl_v8/src/core/lv_event.c
			lvgl_v8/src/core/lv_refr.c
			lvgl_v8/src/core/lv_theme.c
			lvgl_v8/src/draw/lv_draw_arc.c
			lvgl_v8/src/draw/lv_draw.c
			lvgl_v8/src/draw/lv_draw_img.c
			lvgl_v8/src/draw/lv_draw_label.c
			lvgl_v8/src/draw/lv_draw_line.c
			lvgl_v8/src/draw/lv_draw_mask.c
			lvgl_v8/src/draw/lv_draw_rect.c
			lvgl_v8/src/draw/lv_draw_transform.c
			lvgl_v8/src/draw/lv_draw_layer.c
			lvgl_v8/src/draw/lv_draw_triangle.c
			lvgl_v8/src/draw/lv_img_buf.c
			lvgl_v8/src/draw/lv_img_cache.c
			lvgl_v8/src/draw/lv_img_decoder.c
			lvgl_v8/src/draw/sw/lv_draw_sw.c
			lvgl_v8/src/draw/sw/lv_draw_sw_arc.c
			lvgl_v8/src/draw/sw/lv_draw_sw_dither.c
			lvgl_v8/src/draw/sw/lv_draw_sw_gradient.c
			lvgl_v8/src/draw/sw/lv_draw_sw_img.c
			lvgl_v8/src/draw/sw/lv_draw_sw_letter.c
			lvgl_v8/src/draw/sw/lv_draw_sw_line.c
			lvgl_v8/src/draw/sw/lv_draw_sw_polygon.c
			lvgl_v8/src/draw/sw/lv_draw_sw_rect.c
			lvgl_v8/src/draw/sw/lv_draw_sw_transform.c
			lvgl_v8/src/draw/sw/lv_draw_sw_layer.c
			lvgl_v8/src/draw/sw/lv_draw_sw_blend.c
			lvgl_v8/src/extra/layouts/flex/lv_flex.c
			lvgl_v8/src/extra/layouts/grid/lv_grid.c
			lvgl_v8/src/extra/themes/basic/lv_theme_basic.c
			lvgl_v8/src/extra/themes/default/lv_theme_default.c
			lvgl_v8/src/extra/themes/mono/lv_theme_mono.c
			lvgl_v8/src/extra/widgets/animimg/lv_animimg.c
			lvgl_v8/src/extra/widgets/calendar/lv_calendar.c
			lvgl_v8/src/extra/widgets/calendar/lv_calendar_header_arrow.c
			lvgl_v8/src/extra/widgets/calendar/lv_calendar_header_dropdown.c
			lvgl_v8/src/extra/widgets/chart/lv_chart.c
			lvgl_v8/src/extra/widgets/colorwheel/lv_colorwheel.c
			lvgl_v8/src/extra/widgets/imgbtn/lv_imgbtn.c
			lvgl_v8/src/extra/widgets/keyboard/lv_keyboard.c
			lvgl_v8/src/extra/widgets/led/lv_led.c
			lvgl_v8/src/extra/widgets/list/lv_list.c
			lvgl_v8/src/extra/widgets/menu/lv_menu.c
			lvgl_v8/src/extra/widgets/meter/lv_meter.c
			lvgl_v8/src/extra/widgets/msgbox/lv_msgbox.c
			lvgl_v8/src/extra/widgets/span/lv_span.c
			lvgl_v8/src/extra/widgets/spinbox/lv_spinbox.c
			lvgl_v8/src/extra/widgets/spinner/lv_spinner.c
			lvgl_v8/src/extra/widgets/tabview/lv_tabview.c
			lvgl_v8/src/extra/widgets/tileview/lv_tileview.c
			lvgl_v8/src/extra/widgets/win/lv_win.c
			lvgl_v8/src/extra/others/fragment/lv_fragment.c
			lvgl_v8/src/extra/others/fragment/lv_fragment_manager.c
			lvgl_v8/src/extra/others/gridnav/lv_gridnav.c
			lvgl_v8/src/extra/others/ime/lv_ime_pinyin.c
			lvgl_v8/src/extra/others/imgfont/lv_imgfont.c
			lvgl_v8/src/extra/others/monkey/lv_monkey.c
			lvgl_v8/src/extra/others/msg/lv_msg.c
			lvgl_v8/src/extra/others/snapshot/lv_snapshot.c
			lvgl_v8/src/extra/libs/bmp/lv_bmp.c
			lvgl_v8/src/extra/libs/ffmpeg/lv_ffmpeg.c
			lvgl_v8/src/extra/libs/freetype/lv_freetype.c
			lvgl_v8/src/extra/libs/fsdrv/lv_fs_fatfs.c
			lvgl_v8/src/extra/libs/fsdrv/lv_fs_posix.c
			lvgl_v8/src/extra/libs/fsdrv/lv_fs_stdio.c
			# lvgl_v8/src/extra/libs/fsdrv/lv_fs_win32.c
			lvgl_v8/src/extra/libs/gif/gifdec.c
			lvgl_v8/src/extra/libs/gif/lv_gif.c
			lvgl_v8/src/extra/libs/png/lodepng.c
			lvgl_v8/src/extra/libs/png/lv_png.c
			lvgl_v8/src/extra/libs/qrcode/lv_qrcode.c
			lvgl_v8/src/extra/libs/qrcode/qrcodegen.c
			lvgl_v8/src/extra/libs/rlottie/lv_rlottie.c
			lvgl_v8/src/extra/libs/sjpg/lv_sjpg.c
			lvgl_v8/src/extra/libs/sjpg/tjpgd.c
			lvgl_v8/src/extra/lv_extra.c
			lvgl_v8/src/font/lv_font.c
			lvgl_v8/src/font/lv_font_fmt_txt.c
			lvgl_v8/src/font/lv_font_loader.c
			lvgl_v8/src/font/lv_font_dejavu_16_persian_hebrew.c
			lvgl_v8/src/font/lv_font_montserrat_8.c
			lvgl_v8/src/font/lv_font_montserrat_10.c
			lvgl_v8/src/font/lv_font_montserrat_12.c
			lvgl_v8/src/font/lv_font_montserrat_12_subpx.c
			lvgl_v8/src/font/lv_font_montserrat_14.c
			lvgl_v8/src/font/lv_font_montserrat_16.c
			lvgl_v8/src/font/lv_font_montserrat_18.c
			lvgl_v8/src/font/lv_font_montserrat_20.c
			lvgl_v8/src/font/lv_font_montserrat_22.c
			lvgl_v8/src/font/lv_font_montserrat_24.c
			lvgl_v8/src/font/lv_font_montserrat_26.c
			lvgl_v8/src/font/lv_font_montserrat_28.c
			lvgl_v8/src/font/lv_font_montserrat_28_compressed.c
			lvgl_v8/src/font/lv_font_montserrat_30.c
			lvgl_v8/src/font/lv_font_montserrat_32.c
			lvgl_v8/src/font/lv_font_montserrat_34.c
			lvgl_v8/src/font/lv_font_montserrat_36.c
			lvgl_v8/src/font/lv_font_montserrat_38.c
			lvgl_v8/src/font/lv_font_montserrat_40.c
			lvgl_v8/src/font/lv_font_montserrat_42.c
			lvgl_v8/src/font/lv_font_montserrat_44.c
			lvgl_v8/src/font/lv_font_montserrat_46.c
			lvgl_v8/src/font/lv_font_montserrat_48.c
			lvgl_v8/src/font/lv_font_simsun_16_cjk.c
			lvgl_v8/src/font/lv_font_unscii_8.c
			lvgl_v8/src/font/lv_font_unscii_16.c
			lvgl_v8/src/hal/lv_hal_disp.c
			lvgl_v8/src/hal/lv_hal_indev.c
			lvgl_v8/src/hal/lv_hal_tick.c
			lvgl_v8/src/misc/lv_anim.c
			lvgl_v8/src/misc/lv_anim_timeline.c
			lvgl_v8/src/misc/lv_area.c
			lvgl_v8/src/misc/lv_async.c
			lvgl_v8/src/misc/lv_bidi.c
			lvgl_v8/src/misc/lv_color.c
			lvgl_v8/src/misc/lv_fs.c
			lvgl_v8/src/misc/lv_gc.c
			lvgl_v8/src/misc/lv_ll.c
			lvgl_v8/src/misc/lv_log.c
			lvgl_v8/src/misc/lv_lru.c
			lvgl_v8/src/misc/lv_math.c
			lvgl_v8/src/misc/lv_mem.c
			lvgl_v8/src/misc/lv_printf.c
			lvgl_v8/src/misc/lv_style.c
			lvgl_v8/src/misc/lv_style_gen.c
			lvgl_v8/src/misc/lv_timer.c
			lvgl_v8/src/misc/lv_tlsf.c
			lvgl_v8/src/misc/lv_txt.c
			lvgl_v8/src/misc/lv_txt_ap.c
			lvgl_v8/src/misc/lv_utils.c
			lvgl_v8/src/widgets/lv_arc.c
			lvgl_v8/src/widgets/lv_bar.c
			lvgl_v8/src/widgets/lv_btn.c
			lvgl_v8/src/widgets/lv_btnmatrix.c
			lvgl_v8/src/widgets/lv_canvas.c
			lvgl_v8/src/widgets/lv_checkbox.c
			lvgl_v8/src/widgets/lv_dropdown.c
			lvgl_v8/src/widgets/lv_img.c
			lvgl_v8/src/widgets/lv_label.c
			lvgl_v8/src/widgets/lv_line.c
			lvgl_v8/src/widgets/lv_roller.c
			lvgl_v8/src/widgets/lv_slider.c
			lvgl_v8/src/widgets/lv_switch.c
			lvgl_v8/src/widgets/lv_table.c
			lvgl_v8/src/widgets/lv_textarea.c
			lvgl_v8/porting/lv_port_disp.c
			lvgl_v8/porting/lv_port_indev.c
		)
	elseif(CONFIG_LVGL_V9)
		set(incs
			lvgl_v9/.
			lvgl_v9/porting
		)

		file(GLOB_RECURSE srcs
			LIST_DIRECTORIES false
			lvgl_v9/src/*.c
			lvgl_v9/src/*.s
			lvgl_v9/src/*.S
		)

		file(GLOB_RECURSE demo_srcs
			LIST_DIRECTORIES false
			lvgl_v9/demos/*.c
		)

		list(APPEND srcs
			${demo_srcs}
			lvgl_v9/porting/lv_port_disp.c
			lvgl_v9/porting/lv_port_disp_partial.c
			lvgl_v9/porting/lv_port_disp_direct.c
			lvgl_v9/porting/lv_port_disp_full.c
			lvgl_v9/porting/lv_port_indev.c
		)
	endif()

	list(APPEND incs
		common/.
		common/camera_blend
		common/gpu_rotate
		common/hpdma
		common/img_utility
	)

	list(APPEND srcs
		common/lv_mem_adapt.c
		common/lv_vendor.c
		common/camera_blend/lv_camera_blend.c
		common/hpdma/lv_hpdma.c
	)

	if (CONFIG_LV_IMG_UTILITY_CUSTOMIZE)
		list(APPEND srcs
			common/img_utility/lv_img_utility.c
			common/img_utility/lv_jpeg_hw_decode.c
			common/img_utility/lv_jpeg_sw_decode.c
		)
	endif()

	if (CONFIG_VFS)
		list(APPEND srcs
			common/vfs/lv_fs_bk_posix.c
		)
	endif()

	if (CONFIG_LVGL_USE_GPU_ROTATE)
		list(APPEND srcs
			common/gpu_rotate/lv_gpu_rotate.c
		)
	endif()
endif()

# bk_baf is a PUBLIC requirement: the public header lv_baf.h (pulled in via
# lvgl.h) includes <bk_baf_types.h>, so bk_baf's include/ dir must propagate to
# every LVGL consumer. The other deps stay private (used only by lvgl sources).
armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES bk_baf
	PRIV_REQUIRES include bk_vfs avdk_driver bk_gpu
	)

if (CONFIG_LVGL)
	set(_project_lv_conf_custom "${project_dir}/ap/lv_conf_custom.h")
	if(EXISTS "${_project_lv_conf_custom}")
		target_compile_definitions(${COMPONENT_LIB} PUBLIC LV_CONF_PATH="${_project_lv_conf_custom}")
	endif()

	if(CONFIG_LVGL_V9)
		# Keep lv_memset/lv_memcpy from being emitted as wide stores on PSRAM buffers.
		set_source_files_properties(lvgl_v9/src/stdlib/builtin/lv_string_builtin.c
			PROPERTIES COMPILE_OPTIONS "-fno-tree-vectorize"
		)

		# The ASM compiler path does not inherit the AP CPU flags in this build system.
		# Keep the native Helium object compatible with the Cortex-M55 AP image.
		set_source_files_properties(lvgl_v9/src/draw/sw/blend/helium/lv_blend_helium.S
			PROPERTIES COMPILE_OPTIONS "-mcpu=cortex-m55;-mfloat-abi=hard;-mfpu=fpv5-sp-d16;-mcmse"
		)
	endif()

	target_compile_options(${COMPONENT_LIB} PRIVATE -O3)
endif()

