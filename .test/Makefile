SHELL := /usr/bin/env bash

.DEFAULT_GOAL := all

TEST_ROOT := $(CURDIR)
SCRIPTS_DIR := $(TEST_ROOT)/scripts

.PHONY: all default-board-build c2-default-build l3-1-default-build preflight summary clean help

all: default-board-build

default-board-build: c2-default-build l3-1-default-build

c2-default-build:
	@$(SCRIPTS_DIR)/run-default-board-build.sh

l3-1-default-build:
	@BOARD_ID=starrysky-l3-1 \
	TARGET=l3_1 \
	TEMPLATES="asm_hello archinfo asm_gpio crc gpio hello i2c_scan i2c_sgp30 minesweeper ps2 pwm qspi rcu rng rtc smoke_test spi_st7735 spi_st7735_clock spi_st7735_donut st7789 timer wdg" \
	ASM_TEMPLATES="asm_hello asm_gpio" \
	$(SCRIPTS_DIR)/run-default-board-build.sh

preflight:
	@$(SCRIPTS_DIR)/preflight.sh

summary:
	@find "$(TEST_ROOT)/reports/default-board-build" -name summary.txt -type f 2>/dev/null | sort | while read -r summary; do \
		echo "$$summary"; \
		cat "$$summary"; \
		echo; \
	done; \
	if [[ -z "$$(find "$(TEST_ROOT)/reports/default-board-build" -name summary.txt -type f 2>/dev/null | head -1)" ]]; then \
		echo "No default-board-build summary found."; \
	fi

clean:
	@rm -rf $(TEST_ROOT)/work/default-board-build
	@rm -rf $(TEST_ROOT)/reports/default-board-build
	@echo "Cleaned generated default-board-build work and reports."

help:
	@echo "Available targets:"
	@echo "  make                     Run the full implemented test suite"
	@echo "  make all                 Run the full implemented test suite"
	@echo "  make default-board-build Run default board build tests"
	@echo "  make c2-default-build    Run C2 supported-template build matrix"
	@echo "  make l3-1-default-build  Run L3.1 supported-template build matrix"
	@echo "  make preflight           Check local test environment"
	@echo "  make summary             Show latest default-board-build summary"
	@echo "  make clean               Remove generated default-board-build work and reports"
