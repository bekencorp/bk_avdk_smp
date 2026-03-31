set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(toolchain_path "$ENV{TOOLCHAIN_DIR}")
set(CMAKE_AR ${toolchain_path}/arm-none-eabi-ar)
set(CMAKE_C_COMPILER ${toolchain_path}/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER ${toolchain_path}/arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER ${toolchain_path}/arm-none-eabi-gcc)

set(armino_objcopy ${toolchain_path}/arm-none-eabi-objcopy)
set(armino_readelf ${toolchain_path}/arm-none-eabi-readelf)
set(armino_nm ${toolchain_path}/arm-none-eabi-nm)
set(armino_objdump ${toolchain_path}/arm-none-eabi-objdump)
set(armino_toolchain_size ${toolchain_path}/arm-none-eabi-size)

#add the libs in toolchain
link_libraries(libm.a) #contian sin() cos() ...
link_libraries(libgcc.a) #contian __riscv_restore_2  __riscv_save_3 ...
link_libraries(libc.a)#contain memset() memcopy() ....
link_libraries(libnosys.a)#contain _write() ...

set(c_link_options "-mcpu=cortex-m55 -mfloat-abi=hard -mfpu=fpv5-sp-d16 -mcmse")

set(CMAKE_C_FLAGS "${c_link_options} -DWIFI_BLE_COEXIST -DCONFIG_CMAKE=1 -DBK_MAC=1 -Wl,--gc-sections -nostdlib")
set(CMAKE_CXX_FLAGS "${c_link_options} -DWIFI_BLE_COEXIST -DCONFIG_CMAKE=1 -DBK_MAC=1 -Wl,--gc-sections -nostdlib")
