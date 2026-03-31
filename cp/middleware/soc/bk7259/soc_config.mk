# SUPPORT_TRIPLE_CORE := true
SUPPORT_BOOTLOADER := true

ifeq ($(WIN32), 1)
	COMPILER_TOOLCHAIN_PATH := $(ARMINO_BASH_TOOLS_PATH)/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin
else
	COMPILER_TOOLCHAIN_PATH := /opt/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin
endif

