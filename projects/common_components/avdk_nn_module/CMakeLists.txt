set(incs
    include
    src/face_detection
    ../../tflite_micro/gesture_detection/ap/ui/beken_generated
)
set(srcs
	src/core/box.c
	src/core/AvdkDetectionModel.cc
	src/core/AvdkVideoReator.cc
)

if (CONFIG_TFLM_PERSON_DETECTION_V1)
	list(APPEND srcs
		src/tflm_person_detection/tfm_person_detection.c
		src/tflm_person_detection/tflm_person_detection.cc
		src/tflm_person_detection/person_detect_model_data.cc
	)
endif()

if (CONFIG_TFLM_FACE_DETECTION_V1)
	list(APPEND srcs
		src/tflm_face_detection/swift_yolo_1xb16_300e_coco_300_model_data.c
		src/tflm_face_detection/face_detection.cc
		src/tflm_face_detection/tflm_face_detection.c
	)
endif()

if (CONFIG_TFLM_GESTURE_DETECTION_V1)
	list(APPEND srcs
		src/tflm_gesture_detection/gesture_detection_model_data.cc
		src/tflm_gesture_detection/GestureDetectionModel.cc
	)
endif()

if (CONFIG_FACE_DETECTION_V2)
	list(APPEND srcs
		src/face_detection/cfacedetectcnn.c
		src/face_detection/face_detection.c
	)
endif()

if (CONFIG_TFLM_MOBILEFACENET_V1)
	list(APPEND srcs
		src/tflm_mobilefacenet/mobilefacenet.cc
		src/tflm_mobilefacenet/mobilefacenet.c
		src/tflm_mobilefacenet/mobilefacenet_int8_96x112_model_data.c
		src/tflm_mobilefacenet/mobilefacenet_int8_112x112_model_data.c
		src/tflm_mobilefacenet/face_a_96x112_01.c
		src/tflm_mobilefacenet/face_a_96x112_02.c
		src/tflm_mobilefacenet/face_b_96x112_01.c
	)
endif()

if (CONFIG_TFLM_YOLOFACE_V1)
	list(APPEND srcs
		src/tflm_yoloface_detection/yoloface_detect_model_data.cc
		src/tflm_yoloface_detection/YolofaceDetectionModel.cc
	)
endif()

if (CONFIG_TFLM_PALM_DETECTION_V1)
	list(APPEND srcs
		src/tflm_palm_detection/PalmDetectionModel.cc
		src/tflm_palm_detection/palm_detect_model_data.cc
	)
endif()

if (CONFIG_COMMON_LVGL_UI)
    file(GLOB_RECURSE beken_generated_source
        src/ui/beken_generated/*.c
        src/ui/beken_generated/fonts/*.c
        src/ui/beken_generated/custom/*.c
    )

    list(APPEND incs
        src/ui/beken_generated
        src/ui/beken_generated/fonts
        src/ui/beken_generated/custom
    )

    list(APPEND srcs ${beken_generated_source})
endif()

set(priv_req bk_common unity include avdk_utils bk_tflite_micro doorbell_device_service lvgl bk_peripheral)


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

add_definitions(
	-DTF_LITE_STATIC_MEMORY
	-DTF_LITE_DISABLE_X86_NEON
	-DTF_LITE_USE_CTIME
	-DTF_LITE_USE_GLOBAL_CMATH_FUNCTIONS
	-DCMSIS_NN
	-DKERNELS_OPTIMIZED_FOR_SPEED
)
