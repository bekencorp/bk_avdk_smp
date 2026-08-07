export ARMINO_AVDK_DIR := $(CURDIR)
export ARMINO_AP_DIR := $(ARMINO_AVDK_DIR)/ap
export ARMINO_CP_DIR := $(ARMINO_AVDK_DIR)/cp

export ARMINO_TOOLS_PATH :=  $(ARMINO_AVDK_DIR)/tools
export ARMINO_TOOL := $(ARMINO_TOOLS_PATH)/build_tools/armino
export ARMINO_TOOL_WRAPPER := @$(ARMINO_TOOLS_PATH)/build_tools/build.sh


# 1. soc_targets contains all supported SoCs
# 2. cmake_supported_targets contains all targets that can directly
#    passed to armino cmake build system
# 3. cmake_not_supported_targets contains all targets:
#    3.1> armino cmake doesn't support it, only implemented in this
#         Makefile
#    3.2> armino cmake supports it, but has different target name
soc_targets_ap := $(shell find  ap/middleware/soc/ -name "*.defconfig" -exec basename {} \; | cut -f1 -d ".")
soc_targets_cp := $(shell find  cp/middleware/soc/ -name "*.defconfig" -exec basename {} \; | cut -f1 -d ".")

soc_targets = $(soc_targets_ap) $(soc_targets_cp)

ap_menuconfig_targets = $(addsuffix _menuconfig, $(soc_targets_ap))
cp_menuconfig_targets = $(addsuffix _cp_menuconfig, $(soc_targets_cp))

cmake_not_supported_targets = help clean doc ap_doc cp_doc
all_targets = cmake_not_supported_targets soc_targets_cp soc_targets_ap cmake_supported_targets ap_menuconfig_targets cp_menuconfig_targets
export SOC_SUPPORTED_TARGETS_AP := ${soc_targets_ap}
export SOC_SUPPORTED_TARGETS_CP := ${soc_targets_cp}
make_target := $(subst _menuconfig,,$(MAKECMDGOALS))
$(info MAKECMDGOALS is $(MAKECMDGOALS))
make_target := $(subst _cp,,$(make_target))
make_target := $(subst _ap,,$(make_target))
ifeq ($(filter $(make_target),$(soc_targets)),)
  export ARMINO_SOC :=
else
  export ARMINO_SOC := $(make_target)
endif

export CMD_TARGET := $(MAKECMDGOALS)
ifeq ("$(APP_VERSION)", "")
	export APP_VERSION := unknown
else
	export APP_VERSION := $(APP_VERSION)
endif

ifeq ("$(PROJECT)", "")
	export PROJECT := app
else
	export PROJECT := $(PROJECT)
endif


ifeq ("$(PROJECT_DIR)", "")
	export PROJECT_DIR := ${CURDIR}/projects/$(PROJECT)
else
	export PROJECT_DIR := $(PROJECT_DIR)
endif


ifeq ("$(ARMINO_SOC)", "")
ifeq ("$(ARMINO_SOC_LIB)", "")
	ARMINO_SOC := bk7259
	ARMINO_TARGET := $(MAKECMDGOALS)
endif
else
	ARMINO_TARGET := build
endif

export ARMINO_SOC_NAME := $(ARMINO_SOC)

export PROJECT_NAME := $(notdir $(PROJECT_DIR))
ifdef BK_CONFIG_FILE
export CONFIG_SUBTITUTE_FILE := $(BK_CONFIG_FILE).config
export PROJECT_NAME := $(PROJECT_NAME)_$(BK_CONFIG_FILE)
endif

ifneq ("$(BUILD_DIR)", "")
	export PROJECT_BUILD_DIR := $(BUILD_DIR)/$(ARMINO_SOC_NAME)/$(PROJECT_NAME)
else
	export PROJECT_BUILD_DIR := $(CURDIR)/build/$(ARMINO_SOC_NAME)/$(PROJECT_NAME)
endif

ifdef USE_LIBS_DETERMINED_MODE
	export ARMINO_USE_WRAPPER := 1
	export ARMINO_WRAPPER_PATH := $(ARMINO_AVDK_DIR)
	export ARMINO_WRAPPER_NEW_PATH := /armino_avdk_smp
endif

# Jenkins: cap sub-make parallelism (override: BK_JENKINS_JOBS=8 make ...)
ifdef BK_JENKINS_ID
    BK_JENKINS_JOBS ?= 2
    MAKEFLAGS += -j$(BK_JENKINS_JOBS)
endif

ifndef PRINT_SUMMARY
	PRINT_SUMMARY := 1
endif

.PHONY: all_targets

help:
	@echo ""
	@echo " make bkxxx - build soc bkxxx"
	@echo " make bkxxx_ap - build soc bkxxx ap"
	@echo " make bkxxx_cp - build soc bkxxx cp"
	@echo " make all - build all soc"
	@echo " make clean - clean build"
	@echo " make help - display this help info"
	@echo " make doc - generate smp doc and cp doc and ap doc (parallel where possible)"
	@echo " ccache: install ccache and set ARMINO_CCACHE_ENABLE=1 (auto if unset and ccache exists, see build.sh)"
	@echo " Jenkins: set BK_JENKINS_JOBS to override default -j when BK_JENKINS_ID is set"
	@echo " make ap_doc - generate ap doc"
	@echo " make cp_doc - generate cp doc"
	@echo " make smp_doc - generate smp doc"
	@echo " make bkxxxx_ap_menuconfig - ap sdk config"
	@echo " make bkxxxx_cp_menuconfig - cp sdk config"
	@echo ""

common:
	@echo "ARMINO_SOC is set to $(ARMINO_SOC)"
	@echo "ARMINO_TARGET is set to $(ARMINO_TARGET)"
	@echo "armino project path=$(PROJECT_DIR)"
	@echo "armino ap path=$(ARMINO_AP_DIR)"
	@echo "armino cp path=$(ARMINO_CP_DIR)"
	@echo "armino build path=$(PROJECT_BUILD_DIR)"


all: $(soc_targets) $(ARMINO_SOC)_cp

ifeq ($(findstring Windows_NT,$(OS)), Windows_NT)
export WIN32 := 1
PRINT_SUMMARY := 0
else
export WIN32 := 0
endif

BUILD_SMP_PARALLEL_SCRIPT := $(ARMINO_TOOLS_PATH)/build_tools/build_smp_parallel.sh
BK_PY_LIBS_PATH := $(ARMINO_TOOLS_PATH)/env_tools/bk_py_libs
ifeq ($(WIN32),1)
export PYTHONPATH := $(BK_PY_LIBS_PATH);$(PYTHONPATH)
RUN_PYTHON3 = python3
else
export PYTHONPATH := $(BK_PY_LIBS_PATH):$(PYTHONPATH)
RUN_PYTHON3 = python3
endif

$(ARMINO_SOC)_ap: common build_prepare
	@make $(ARMINO_SOC)_ap ARMINO_TOOLS_PATH=$(ARMINO_TOOLS_PATH) PROJECT_DIR=$(PROJECT_DIR) BUILD_DIR=$(PROJECT_BUILD_DIR) APP_NAME=$(APP_NAME) APP_VERSION=$(APP_VERSION) -C $(ARMINO_AP_DIR)

$(ARMINO_SOC)_cp: common build_prepare
	@make $(ARMINO_SOC) ARMINO_TOOLS_PATH=$(ARMINO_TOOLS_PATH) PROJECT_DIR=$(PROJECT_DIR) BUILD_DIR=$(PROJECT_BUILD_DIR) APP_NAME=$(APP_NAME) APP_VERSION=$(APP_VERSION) -C $(ARMINO_CP_DIR)

# Parallel AP+CP with fail-fast (see build_smp_parallel.sh). Windows keeps legacy make -j behavior.
ifeq ($(WIN32),1)
$(soc_targets_cp): $(ARMINO_SOC)_ap $(ARMINO_SOC)_cp package
else
build_smp_firmware: common build_prepare
	@ARMINO_SOC=$(ARMINO_SOC) ARMINO_TOOLS_PATH=$(ARMINO_TOOLS_PATH) \
		PROJECT_DIR=$(PROJECT_DIR) BUILD_DIR=$(PROJECT_BUILD_DIR) \
		APP_NAME=$(APP_NAME) APP_VERSION=$(APP_VERSION) \
		ARMINO_AP_DIR=$(ARMINO_AP_DIR) ARMINO_CP_DIR=$(ARMINO_CP_DIR) \
		PYTHONPATH=$(BK_PY_LIBS_PATH):$$PYTHONPATH \
		bash $(BUILD_SMP_PARALLEL_SCRIPT)

$(soc_targets_cp): build_smp_firmware package
endif

DOCS_PARAMTERS:=
ifneq ("$(DOCS_TARGET)", "")
	DOCS_PARAMTERS += --target $(DOCS_TARGET)
endif
ifneq ("$(DOCS_TYPE)", "")
	DOCS_PARAMTERS += --type $(DOCS_TYPE)
endif
ifneq ("$(DOCS_VERSION)", "")
	DOCS_PARAMTERS += --version $(DOCS_VERSION)
endif

AUTO_PARTITION_TABLE := $(PROJECT_DIR)/partitions/$(ARMINO_SOC_NAME)/auto_partitions.csv
export PARTITIONS_DIR := $(PROJECT_BUILD_DIR)/partitions
auto_partition_script := $(ARMINO_AVDK_DIR)/tools/build_tools/build_process/bk_build_auto_partition.py
auto_partition_out := $(PARTITIONS_DIR)/partitions.txt

$(auto_partition_out): $(auto_partition_script) $(AUTO_PARTITION_TABLE)
	@mkdir -p $(PARTITIONS_DIR)
	@$(RUN_PYTHON3) $(auto_partition_script)

print_partitions: $(auto_partition_out)
	@echo ===================== Partitions Table =====================
	@cat $(auto_partition_out)
	@echo ============================================================

ram_partition_script := $(ARMINO_AVDK_DIR)/tools/build_tools/build_process/bk_build_ram_regions.py
RAM_REGIONS_DIR := $(PROJECT_DIR)/partitions/$(ARMINO_SOC_NAME)
RAM_REGIONS_TABLE := $(RAM_REGIONS_DIR)/ram_regions.csv
RAM_REGIONS_MPU_POLICY := $(wildcard $(RAM_REGIONS_DIR)/ram_regions_mpu.json)
ram_regions_out := $(PARTITIONS_DIR)/ram_regions.h
ram_regions_setting := $(firstword \
	$(wildcard $(ARMINO_AVDK_DIR)/tools/build_tools/build_process/bk_sdk/smp_ram_setting_$(ARMINO_SOC_NAME).json) \
	$(ARMINO_AVDK_DIR)/tools/build_tools/build_process/bk_sdk/smp_ram_setting.json)
$(ram_regions_out): $(RAM_REGIONS_DIR) $(RAM_REGIONS_TABLE) $(RAM_REGIONS_MPU_POLICY) $(ram_partition_script) $(ram_regions_setting)
	@mkdir -p $(PARTITIONS_DIR)
	@$(RUN_PYTHON3) $(ram_partition_script)

# Clean bootloader once before the AP/CP build to ensure a full rebuild.
NORMAL_BOOTLOADER_DIR := $(ARMINO_CP_DIR)/properties/modules/bootloader/aboot/arm_bootloader
.PHONY: bootloader_fullclean
bootloader_fullclean:
	@echo "[full-build] clean bootloader once for a reproducible full build"
	@if [ -d "$(NORMAL_BOOTLOADER_DIR)" ]; then \
		$(MAKE) -C $(NORMAL_BOOTLOADER_DIR) SOC_TYPE=$(ARMINO_SOC) clean; \
	fi

build_prepare: $(auto_partition_out) print_partitions $(ram_regions_out) bootloader_fullclean

package_script := $(ARMINO_AVDK_DIR)/tools/build_tools/build_process/bk_build_package.py
package_dir := $(PROJECT_BUILD_DIR)/package
package_json := $(PARTITIONS_DIR)/bk_package.json
build_summary := $(package_dir)/build_summary.txt

# Secure firmware detection: when the project enables CONFIG_SECURITY_FIRMWARE,
# the per-subsystem secure pack (board wrapper -> beken_utils) already produces
# the signed/encrypted all-app.bin + bootloader.bin during build_smp_firmware.
# The generic SMP packager (bk_build_package.py) would re-combine raw
# per-partition bins (primary_tfm_s.bin ...) and is incompatible with the
# secure flow, so skip it for secure builds.
SECURITY_CONFIG_FILE := $(PROJECT_DIR)/config/$(ARMINO_SOC_NAME)/config
IS_SECURITY_FIRMWARE := $(shell test -f $(SECURITY_CONFIG_FILE) && grep -q '^CONFIG_SECURITY_FIRMWARE=y' $(SECURITY_CONFIG_FILE) && echo y)
secure_install_dir := $(PROJECT_BUILD_DIR)/$(ARMINO_SOC)/install

ifeq ($(WIN32),1)
package: $(package_script) $(ARMINO_SOC)_cp $(ARMINO_SOC)_ap
else
package: $(package_script) build_smp_firmware
endif
ifeq ($(IS_SECURITY_FIRMWARE),y)
	@echo "Secure firmware: staging $(secure_install_dir) -> $(package_dir) (generic SMP packager skipped)."
	@if [ ! -d "$(secure_install_dir)" ]; then \
		echo "ERROR: secure install dir not found: $(secure_install_dir)"; \
		echo "       CP secure pack (bk7259.wrapper) may not have run."; \
		exit 1; \
	fi
	@mkdir -p $(package_dir)
	@cp -a $(secure_install_dir)/. $(package_dir)/
	@echo "firmware: $(package_dir)/all-app.bin" > $(build_summary)
	@echo "bootloader: $(package_dir)/bootloader.bin" >> $(build_summary)
	@echo "ota binary: $(package_dir)/ota.bin" >> $(build_summary)
ifneq ($(PRINT_SUMMARY), 0)
	@cat $(build_summary)
endif
else
	@mkdir -p $(package_dir)
	@$(RUN_PYTHON3) $(package_script) $(PROJECT_BUILD_DIR) $(package_json) $(build_summary)
ifneq ($(PRINT_SUMMARY), 0)
	@cat $(build_summary)
endif
endif

.PHONY: smp_doc ap_doc cp_doc doc build_smp_firmware

ap_doc:
	@make doc ARMINO_TOOLS_PATH=$(ARMINO_TOOLS_PATH) -C $(ARMINO_AP_DIR)

cp_doc:
	@make doc ARMINO_TOOLS_PATH=$(ARMINO_TOOLS_PATH) -C $(ARMINO_CP_DIR)

smp_doc:
	@ARMINO_SOC=$${ARMINO_SOC:-bk7259} ARMINO_AVDK_DIR=$(ARMINO_AVDK_DIR) \
		$(RUN_PYTHON3) ./tools/armino_doc.py $(DOCS_PARAMTERS)

# Run SMP / AP / CP doc in parallel (total wall time ~ max, not sum).
doc:
	+@$(MAKE) -j3 smp_doc ap_doc cp_doc

# only build bootloader
bootloader_build_script := $(ARMINO_AVDK_DIR)/tools/build_tools/build_process/bk_sdk/bl_build.py
bl:
	@python $(bootloader_build_script) $(PROJECT_DIR) $(CURDIR)/build $(ARMINO_SOC)

$(ARMINO_SOC)_ap_menuconfig: common
	@make menuconfig ARMINO_TOOLS_PATH=$(ARMINO_TOOLS_PATH) PROJECT_DIR=$(PROJECT_DIR) BUILD_DIR=$(PROJECT_BUILD_DIR) APP_NAME=$(APP_NAME) APP_VERSION=$(APP_VERSION) MENUCONFIG_DEST_TYPE=ap SOC_NAME=$(ARMINO_SOC) -C $(ARMINO_AP_DIR)

$(ARMINO_SOC)_cp_menuconfig: common
	@make menuconfig ARMINO_TOOLS_PATH=$(ARMINO_TOOLS_PATH) PROJECT_DIR=$(PROJECT_DIR) BUILD_DIR=$(PROJECT_BUILD_DIR) APP_NAME=$(APP_NAME) APP_VERSION=$(APP_VERSION) MENUCONFIG_DEST_TYPE=cp SOC_NAME=$(ARMINO_SOC) -C $(ARMINO_CP_DIR)
clean:
	@echo "rm -rf ./build"
	@$(RUN_PYTHON3) ./tools/armino_doc.py --clean True
	@rm -rf ./build
	@rm -rf $(ARMINO_AP_DIR)/build
	@rm -rf $(ARMINO_CP_DIR)/build
	@echo "clean bootloader output"
	@if [ -d "$(NORMAL_BOOTLOADER_DIR)" ]; then \
		$(MAKE) -C $(NORMAL_BOOTLOADER_DIR) SOC_TYPE=$(ARMINO_SOC) clean; \
	fi

