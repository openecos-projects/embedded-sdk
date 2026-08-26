# 所有外部程序只读复制到当前工程 build/staging 后再构建。
STAGE_DIR := $(BUILD_DIR)/staging/$(APP)
WORK_DIR := $(BUILD_DIR)/work/$(APP)
IMAGE_DIR := $(BUILD_DIR)/images/$(APP)

.PHONY: stage
stage:
	@rm -rf "$(STAGE_DIR)"
	@mkdir -p "$(STAGE_DIR)"
	@cp -a "$(AM_KERNELS_DIR)/$(PROGRAM_PATH)/." "$(STAGE_DIR)/"
	@find "$(STAGE_DIR)" -type d -name build -prune -exec rm -rf -- {} +
	@find "$(STAGE_DIR)" -type f \( -name .result -o -name 'Makefile.*' \) -delete
