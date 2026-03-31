set(incs .)
set(srcs ap_main.c)

# Person detection is the only enabled model in this project.
list(APPEND incs person_detection)
list(APPEND srcs
	person_detection/tflm_person_detection_demo.cpp
	person_detection/tflm_person_detection_image_provider.cc
	person_detection/tflm_person_detection_model_vela_data.cc
)

set(priv_req bk_tflite_micro bk_frame_buffer avdk_utils media_service)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
