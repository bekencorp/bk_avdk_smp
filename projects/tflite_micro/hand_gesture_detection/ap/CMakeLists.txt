set(incs . app resource)
set(srcs ap_main.c)

list(APPEND srcs
	app/tflm_hand_gesture_detection_demo.cpp
	resource/hand_gesture_image_input_1.cc
	resource/hand_gesture_image_input_2.cc
)

set(priv_req bk_tflite_micro bk_frame_buffer avdk_utils detection_service avdk_nn_module media_service doorbell_device_service bk_peripheral)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
