set(incs .)
set(srcs ap_main.c)

# Person detection is the only enabled model in this project.
list(APPEND incs person_detection)
list(APPEND srcs
	person_detection/tflm_person_detection_demo.cpp
	person_detection/tflm_person_detection_image_provider.cc
	person_detection/tflm_person_detection_model_vela_data.cc
)

# Dual-interpreter (person + pet) parallel demo. The pet model data file is a
# self-contained copy from avdk_nn_module so this project does not inherit the
# avdk_nn_module multimedia dependency chain (lvgl / multimedia_device_service /
# bk_peripheral) just for the model symbols.
list(APPEND incs multi_model)
list(APPEND srcs
	multi_model/tflm_multi_model_demo.cpp
	multi_model/tflm_pet_postprocess.cpp
	multi_model/pet_detect_model_data.cc
	multi_model/pet_image_input_1.cc
	multi_model/pet_image_input_2.cc
)

set(priv_req bk_tflite_micro bk_frame_buffer avdk_utils media_service)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
