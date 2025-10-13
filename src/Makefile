# retroSoC 固件构建脚本
# 用于编译基于RISC-V架构的嵌入式固件

# RISC-V 交叉编译工具链前缀
CROSS=riscv32-unknown-elf-

# 编译器标志配置
# -mabi=ilp32: 使用ILP32 ABI
# -march=rv32im: RV32I基础指令集 + M乘除法扩展
# -ffreestanding: 独立环境编译
# -nostdlib: 不链接标准库
CFLAGS := -mabi=ilp32 \
	  -march=rv32im \
	  -Wl,-Bstatic,-T,retrosoc_sections.lds,--strip-debug \
	  -ffreestanding \
	  -nostdlib

# 固件名称
FIRMWARE_NAME := retrosoc_fw

# 源文件列表
SRC_PATH := ./start.s \
	    ./tinyuart.c \
	    ./tinystring.c \
	    ./tinyprintf.c \
	    ./tinyflash.c \
	    ./tinygpio.c \
	    ./tinyhkspi.c \
	    ./tinytim.c \
	    ./tinyarchinfo.c \
	    ./tinyrng.c \
	    ./tinyhpuart.c \
	    ./tinypwm.c \
	    ./tinyps2.c \
	    ./tinyi2c.c \
	    ./tinylcd.c \
	    ./tinypsram.c \
	    ./tinybench.c \
	    ./tinysh.c \
	    ./tinydonut.c \
	    ./donut_tft.c \
	    ./firmware.c

# 链接脚本路径
LDS_PATH := ./sections.lds

# 主要构建目标：生成固件的所有格式文件
$(FIRMWARE_NAME).elf:
	$(CROSS)cpp -P -o retrosoc_sections.lds $(LDS_PATH)
	$(CROSS)gcc $(CFLAGS) -I./ -o $@ $(SRC_PATH)
	$(CROSS)objcopy -O verilog $@ $(FIRMWARE_NAME).hex
	sed -i 's/@30000000/@00000000/g' retrosoc_fw.hex
	$(CROSS)objcopy -O binary  $@ $(FIRMWARE_NAME).bin
	$(CROSS)objdump -d $@ > $(FIRMWARE_NAME).txt

# 清理构建产物
clean:
	rm -f *.elf *.hex *.bin *.txt

# 声明伪目标
.PHONY: $(FIRMWARE_NAME).elf clean
