set(CUR_DIR ${CMAKE_CURRENT_LIST_DIR})
set(src 
    ${CUR_DIR}/Driver_Flash.c
    ${CUR_DIR}/Driver_USART.c
)
target_sources(platform_s PRIVATE ${src})

if (BL2)
    target_sources(platform_bl2 PRIVATE ${src})
endif()