# pm_secureboot_ap_fast_boot project
# CP secure boot (BootROM -> CP BL2 -> CP TF-M Secure -> CP NS)
# plus AP fast resume after AP power-off.
cmake_minimum_required(VERSION 3.5)

# Reuse the SDK example signing keys from the base secure-boot project without
# duplicating a private key in this project.
set(EXAMPLE_KEY_SOURCE "${CMAKE_CURRENT_LIST_DIR}/../../secureboot_overwrite_no_en/config/key")
set(PROJECT_KEY_DIR "${CMAKE_CURRENT_LIST_DIR}/config/key")
file(MAKE_DIRECTORY "${PROJECT_KEY_DIR}")
file(COPY
    "${EXAMPLE_KEY_SOURCE}/root_ec256_pubkey.pem"
    "${EXAMPLE_KEY_SOURCE}/root_ec256_privkey.pem"
    DESTINATION "${PROJECT_KEY_DIR}"
)

include($ENV{ARMINO_TOOLS_PATH}/build_tools/cmake/project.cmake)
project(pm_secureboot_ap_fast_boot)
