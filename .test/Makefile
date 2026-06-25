SHELL := /usr/bin/env bash

.DEFAULT_GOAL := all

TEST_ROOT := $(CURDIR)
SCRIPTS_DIR := $(TEST_ROOT)/scripts

.PHONY: all default-board-build c2-default-build preflight summary clean help

all: default-board-build

default-board-build: c2-default-build

c2-default-build:
	@$(SCRIPTS_DIR)/run-default-board-build.sh

preflight:
	@$(SCRIPTS_DIR)/preflight.sh

summary:
	@latest=$$(find "$(TEST_ROOT)/reports/default-board-build" -path '*/starrysky-c2/summary.txt' -type f 2>/dev/null | sort | tail -1); \
	if [[ -n "$$latest" ]]; then \
		echo "$$latest"; \
		cat "$$latest"; \
	else \
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
	@echo "  make preflight           Check local test environment"
	@echo "  make summary             Show latest default-board-build summary"
	@echo "  make clean               Remove generated default-board-build work and reports"
