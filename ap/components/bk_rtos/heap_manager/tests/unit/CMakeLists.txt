cmake_minimum_required(VERSION 3.15)

# Project configuration
project(bk_heap_test VERSION 1.0.0 LANGUAGES C CXX)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find gtest
find_package(GTest REQUIRED)

# Note: We don't set include_directories here to avoid inheriting from parent.
# Instead, we use target_include_directories below to have full control.

# Collect test source files
file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
message(STATUS "Test sources: ${TEST_SOURCES}")

# Collect source files to test
file(GLOB BK_HEAP_SOURCES
    "../../bk_heap/src/*.c"
    "../../bk_heap_impl/*.c"
)

# Collect Mock source files
file(GLOB MOCK_SOURCES "mock/*.c")

# Set compile options (enable test macros)
add_definitions(-DMOCK_ASSERT_ABORT=0)  # Mock assertion does not abort, only records
# Note: MOCK_PORT_HEAP_H is no longer needed - test/unit/mock/port_heap.h handles this
add_definitions(-DBK_HEAP_TEST_ENV=1)

# Create test executable
add_executable(bk_heap_test
    ${TEST_SOURCES}
    ${BK_HEAP_SOURCES}
    ${MOCK_SOURCES}
)

# Set include directories for test target
target_include_directories(bk_heap_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/mock
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bk_heap/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bk_heap/config
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bk_heap_impl
)

# Link libraries
target_link_libraries(
    bk_heap_test
    ${GTEST_BOTH_LIBRARIES}
    pthread
)

# Add tests
include(CTest)
enable_testing()
add_test(NAME unit_test COMMAND bk_heap_test)

