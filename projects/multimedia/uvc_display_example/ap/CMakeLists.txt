set(incs . include)

set(srcs
	ap_main.c
	src/uvc_test.c
	src/encode_frame_que.c
	src/decode_test.c
	src/display_test.c
	src/gpu_test_v2.c
	src/pipeline_test.c
)

set(priv_req media_service bk_uvc bk_peripheral bk_h264d avdk_utils bk_gpu bk_h264e)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")