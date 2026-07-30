cmake_policy(SET CMP0076 NEW)

add_subdirectory(../common bk_common)
add_subdirectory(board)
add_subdirectory(idk)
add_subdirectory(armino_min)
add_subdirectory(partition)
add_subdirectory(startup)
add_subdirectory(stub)
add_subdirectory(ld)

include(ns/ns_install.cmake)
