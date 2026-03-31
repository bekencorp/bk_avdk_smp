set(incs . resource/prompt_tone)
set(srcs
    ap_main.cc
    app_event.c
    app_event_test_cli.c
)

# Chinese prompt tone sources (CN)
set(prompt_tone_srcs_cn
    resource/prompt_tone/cn/cant_det.c
    resource/prompt_tone/cn/detecting.c
    resource/prompt_tone/cn/draw.c
    resource/prompt_tone/cn/game_rule.c
    resource/prompt_tone/cn/get_ready.c
    resource/prompt_tone/cn/my_show.c
    resource/prompt_tone/cn/paper.c
    resource/prompt_tone/cn/pls_stop_hand.c
    resource/prompt_tone/cn/rock.c
    resource/prompt_tone/cn/scissors.c
    resource/prompt_tone/cn/you_loss.c
    resource/prompt_tone/cn/you_win.c
    resource/prompt_tone/cn/your_show.c
)

# English prompt tone sources (EN)
set(prompt_tone_srcs_en
    resource/prompt_tone/en/cant_det.c
    resource/prompt_tone/en/detecting.c
    resource/prompt_tone/en/draw.c
    resource/prompt_tone/en/game_rule.c
    resource/prompt_tone/en/get_ready.c
    resource/prompt_tone/en/my_show.c
    resource/prompt_tone/en/paper.c
    resource/prompt_tone/en/pls_stop_hand.c
    resource/prompt_tone/en/rock.c
    resource/prompt_tone/en/scissors.c
    resource/prompt_tone/en/you_loss.c
    resource/prompt_tone/en/you_win.c
    resource/prompt_tone/en/your_show.c
)

if(CONFIG_GESTURE_DET_PROMPT_TONE_LANG_CHINESE)
    list(APPEND srcs ${prompt_tone_srcs_cn})
elseif(CONFIG_GESTURE_DET_PROMPT_TONE_LANG_ENGLISH)
    list(APPEND srcs ${prompt_tone_srcs_en})
endif()

file(GLOB_RECURSE beken_generated_source
    ui/beken_generated/*.c
    ui/beken_generated/fonts/*.c
    ui/beken_generated/custom/*.c
    ui/beken_generated/image/*.c
)

list(APPEND incs
    ui/beken_generated
    ui/beken_generated/fonts
    ui/beken_generated/custom
)

list(APPEND srcs ${beken_generated_source})

set(priv_req bk_tflite_micro bk_frame_buffer avdk_utils detection_service avdk_nn_module media_service doorbell_device_service bk_peripheral bk_player_service bk_cli lvgl)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")


