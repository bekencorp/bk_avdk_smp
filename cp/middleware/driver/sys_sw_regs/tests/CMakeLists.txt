cmake_minimum_required(VERSION 3.15)

project(test_sys_sw_regs)

find_package(GTest REQUIRED)

include(CTest)
enable_testing()

add_executable(test_sys_sw_regs
    test_sys_sw_regs.cpp
    ../sys_sw_regs.c
)

target_include_directories(test_sys_sw_regs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/mock
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(test_sys_sw_regs ${GTEST_BOTH_LIBRARIES} pthread)

add_test(NAME ut COMMAND test_sys_sw_regs)
