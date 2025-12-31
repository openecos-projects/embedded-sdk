# Memory Usage Reporting Macro
# Usage: $(call show_mem_usage, <elf_file_path>)

define show_mem_usage
	@echo "------------------------------------------------------------------------------"
	@echo "Memory Usage:"
	@$(CROSS)objdump -h $(1) > $(dir $(1))sections.info
	@echo "--------------------------------------------------------------------------"
	@printf "%-15s %-15s %-15s %-10s %-15s\n" "Memory Region" "Used Size" "Total Size" "Usage %" "Free"
	@echo "--------------------------------------------------------------------------"
	@text_size=$$(grep " .text " $(dir $(1))sections.info | awk '{print strtonum("0x"$$3)}'); \
	data_size=$$(grep " .data " $(dir $(1))sections.info | awk '{print strtonum("0x"$$3)}'); \
	bss_size=$$(grep " .bss " $(dir $(1))sections.info | awk '{print strtonum("0x"$$3)}'); \
	data_vma=$$(grep " .data " $(dir $(1))sections.info | awk '{print strtonum("0x"$$4)}'); \
	if [ -z "$$text_size" ]; then text_size=0; fi; \
	if [ -z "$$data_size" ]; then data_size=0; fi; \
	if [ -z "$$bss_size" ]; then bss_size=0; fi; \
	if [ -z "$$data_vma" ]; then data_vma=$$(grep " .bss " $(dir $(1))sections.info | awk '{print strtonum("0x"$$4)}'); fi; \
	if [ -z "$$data_vma" ]; then data_vma=0; fi; \
	flash_total=$$((16 * 1024 * 1024)); \
	flash_used=$$((text_size + data_size)); \
	flash_free=$$((flash_total - flash_used)); \
	flash_pct=$$(awk "BEGIN {printf \"%.2f\", ($$flash_used / $$flash_total) * 100}"); \
	flash_used_kb=$$(awk "BEGIN {if ($$flash_used < 1024) printf \"%d B\", $$flash_used; else if ($$flash_used < 1048576) printf \"%.2f KB\", $$flash_used/1024; else printf \"%.2f MB\", $$flash_used/1048576}"); \
	flash_total_mb=$$(awk "BEGIN {printf \"%.2f MB\", $$flash_total/1048576}"); \
	flash_free_mb=$$(awk "BEGIN {printf \"%.2f MB\", $$flash_free/1048576}"); \
	printf "%-15s %-15s %-15s %-10s %-15s\n" "FLASH" "$$flash_used_kb" "$$flash_total_mb" "$$flash_pct%" "$$flash_free_mb"; \
	ram_type="Unknown"; \
	ram_total=0; \
	if [ $$data_vma -ge $$((0x30000000)) ] && [ $$data_vma -lt $$((0x30020000)) ]; then \
		ram_type="SRAM"; \
		ram_total=$$((128 * 1024)); \
	elif [ $$data_vma -ge $$((0x40000000)) ] && [ $$data_vma -lt $$((0x40800000)) ]; then \
		ram_type="PSRAM"; \
		ram_total=$$((8 * 1024 * 1024)); \
	fi; \
	ram_used=$$((data_size + bss_size)); \
	ram_free=$$((ram_total - ram_used)); \
	if [ "$$ram_type" != "Unknown" ]; then \
		ram_pct=$$(awk "BEGIN {printf \"%.2f\", ($$ram_used / $$ram_total) * 100}"); \
		ram_used_kb=$$(awk "BEGIN {if ($$ram_used < 1024) printf \"%d B\", $$ram_used; else if ($$ram_used < 1048576) printf \"%.2f KB\", $$ram_used/1024; else printf \"%.2f MB\", $$ram_used/1048576}"); \
		ram_total_kb=$$(awk "BEGIN {if ($$ram_total < 1048576) printf \"%.2f KB\", $$ram_total/1024; else printf \"%.2f MB\", $$ram_total/1048576}"); \
		ram_free_kb=$$(awk "BEGIN {if ($$ram_free < 1048576) printf \"%.2f KB\", $$ram_free/1024; else printf \"%.2f MB\", $$ram_free/1048576}"); \
		printf "%-15s %-15s %-15s %-10s %-15s\n" "$$ram_type" "$$ram_used_kb" "$$ram_total_kb" "$$ram_pct%" "$$ram_free_kb"; \
		echo "--------------------------------------------------------------------------"; \
		echo "RAM Details:"; \
		data_kb=$$(awk "BEGIN {if ($$data_size < 1024) printf \"%d B\", $$data_size; else printf \"%.2f KB\", $$data_size/1024}"); \
		bss_kb=$$(awk "BEGIN {if ($$bss_size < 1024) printf \"%d B\", $$bss_size; else printf \"%.2f KB\", $$bss_size/1024}"); \
		echo "  .data: $$data_kb"; \
		echo "  .bss:  $$bss_kb"; \
	else \
		ram_used_kb=$$(awk "BEGIN {if ($$ram_used < 1024) printf \"%d B\", $$ram_used; else printf \"%.2f KB\", $$ram_used/1024}"); \
		printf "%-15s %-15s %-15s %-10s %-15s\n" "RAM (Unknown)" "$$ram_used_kb" "?" "?" "?"; \
		echo "--------------------------------------------------------------------------"; \
		echo "RAM Details:"; \
		data_kb=$$(awk "BEGIN {if ($$data_size < 1024) printf \"%d B\", $$data_size; else printf \"%.2f KB\", $$data_size/1024}"); \
		bss_kb=$$(awk "BEGIN {if ($$bss_size < 1024) printf \"%d B\", $$bss_size; else printf \"%.2f KB\", $$bss_size/1024}"); \
		echo "  .data: $$data_kb"; \
		echo "  .bss:  $$bss_kb"; \
		if [ $$data_vma -ne 0 ]; then printf "  VMA:   0x%08x\n" $$data_vma; fi; \
	fi
	@echo "------------------------------------------------------------------------------"
endef
