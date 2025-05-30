PRE_BUILD_TARGET :=

ifeq ($(SUPPORT_BOOTLOADER),true)
	PRE_BUILD_TARGET += bootloader
	ARMINO_BOOTLOADER = $(ARMINO_DIR)/properties/modules/bootloader/aboot/arm_bootloader_ab
endif
