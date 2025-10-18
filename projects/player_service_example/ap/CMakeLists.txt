set(incs include  ./player_service_test ./voice_service_test)
set(srcs ap_main.c)

list(APPEND srcs
    player_service_test/cli_player_service.c
)

if (CONFIG_VOICE_SERVICE_TEST AND CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE)
    list(APPEND srcs
        voice_service_test/cli_voice_service.c
        )
endif()

set(priv_req media_service avdk_utils)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")

# Copy .it.csv file to build directory
file(COPY ${CMAKE_CURRENT_LIST_DIR}/../.it.csv DESTINATION ${CMAKE_BINARY_DIR}/)