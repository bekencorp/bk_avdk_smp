set(incs include)
set(srcs
	src/bk_osd_emwin_font.c
)

if (CONFIG_VG_LITE_GPU)
list(APPEND srcs
	src/bk_draw_osd.c
	src/bk_draw_osd_ctlr.c
	src/bk_osd_engine.c
	src/bk_osd_lv_font.c
)
endif()

armino_component_register(
	SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES avdk_driver media_service avdk_utils bk_frame_buffer bk_gpu
)
