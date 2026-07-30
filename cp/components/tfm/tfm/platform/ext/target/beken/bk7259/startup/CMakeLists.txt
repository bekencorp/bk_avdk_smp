target_sources(platform_s PRIVATE startup_bk7259_s.c)
if(BL2)
    target_sources(platform_bl2 PRIVATE
        startup_bk7259_bl2.c
        bl2_board_clock.c
    )
endif()