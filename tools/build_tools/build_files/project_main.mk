PROJECT_PATH := $(CURDIR)

PROJECT_NAME := $(notdir $(PROJECT_PATH))
TARGET := $(MAKECMDGOALS)

ifeq ($(findstring Windows_NT,$(OS)), Windows_NT)
PROJECT_PATH := $(subst \,/,$(PROJECT_PATH))
SDK_DIR := $(subst \,/,$(SDK_DIR))
endif

ifeq ("$(TARGET)", "clean")
SOC_TARGET := dummy
else
SOC_TARGET := $(TARGET)
$(info "Project: $(PROJECT_NAME)")
$(info "SDK_DIR: $(SDK_DIR)")
$(info "TARGET: $(TARGET)")
endif

ifeq ("$(SOC_TARGET)", "")
$(error "please input soc target")
endif

PROJECT_BUILD_DIR := $(PROJECT_PATH)/build
SDK_PY_LIBS := $(SDK_DIR)/tools/env_tools/bk_py_libs

.PHONY: clean

$(SOC_TARGET):
	@$(MAKE) $(SOC_TARGET) PROJECT=$(PROJECT_NAME) PROJECT_DIR=$(PROJECT_PATH) BUILD_DIR=$(PROJECT_BUILD_DIR) \
		PYTHONPATH=$(SDK_PY_LIBS):$$PYTHONPATH -C $(SDK_DIR)

clean:
	@echo "rm -rf ./build"
	@rm -rf ./build
