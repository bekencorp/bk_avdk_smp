target_sources(platform_s PRIVATE
    startup_bk7259_s.c
    deepsleep_fastboot_retention.c
)
if(CONFIG_SLEEP_RETENTION_NSC)
    target_sources(platform_s PRIVATE
        sleep_fastboot_tfm.c
    )
endif()
if(BL2)
    target_sources(platform_bl2 PRIVATE
        startup_bk7259_bl2.c
        deepsleep_fastboot_bl2.c
        bl2_board_clock.c
    )
endif()