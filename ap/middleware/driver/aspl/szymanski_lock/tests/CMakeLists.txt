cmake_minimum_required(VERSION 3.15)
project(szymanski_lock_tests C CXX)

find_package(GTest REQUIRED)
include(CTest)
enable_testing()

set(SZYMANSKI_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../src/szymanski_lock.c")
set_source_files_properties(${SZYMANSKI_SRC} PROPERTIES COMPILE_FLAGS "-std=gnu99")

add_executable(szymanski_test
    szymanski_lock_test.cpp
    ${SZYMANSKI_SRC}
)
target_include_directories(szymanski_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")
target_link_libraries(szymanski_test ${GTEST_BOTH_LIBRARIES} pthread)

add_test(NAME szymanski_test COMMAND szymanski_test)
