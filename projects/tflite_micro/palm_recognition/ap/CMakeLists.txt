set(incs .)
set(srcs ap_main.cc app_event.c car_control.c)

set(priv_req bk_tflite_micro bk_frame_buffer avdk_utils detection_service avdk_nn_module media_service doorbell_device_service bk_peripheral)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")


