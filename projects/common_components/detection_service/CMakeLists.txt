set(incs
    include
)
set(srcs 
	src/aov_camera.c
	src/aov_detection_utils.c
	src/aov_detection.c
)

set(priv_req bk_common)

list(APPEND priv_req unity include avdk_utils
	bk_tflite_micro doorbell_device_service avdk_nn_module
)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

add_definitions(-Wno-unused-variable
				-Wno-unused-but-set-variable
				-Wno-int-to-pointer-cast
				-Wno-pointer-to-int-cast)
