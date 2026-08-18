# secureboot_xip project
# CP secure boot (BootROM -> CP BL2 -> CP TF-M Secure -> CP NS)
cmake_minimum_required(VERSION 3.5)

include($ENV{ARMINO_TOOLS_PATH}/build_tools/cmake/project.cmake)
project(secureboot_xip)
