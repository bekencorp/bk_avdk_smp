# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENTS_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common_components/avdk_nn_module
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common_components/multimedia_device_service
)

include($ENV{ARMINO_TOOLS_PATH}/build_tools/cmake/project.cmake)
project(app)
