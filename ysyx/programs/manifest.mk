# ysyx-am 程序授权清单。目录存在不代表程序获得构建授权。
YSYX_PROGRAMS := \
	benchmarks/coremark \
	benchmarks/dhrystone \
	benchmarks/microbench \
	kernels/bad-apple \
	kernels/blockchain \
	kernels/demo \
	kernels/hello \
	kernels/litenes \
	kernels/nemu \
	kernels/slider \
	kernels/snake \
	kernels/thread-os \
	kernels/typing-game \
	kernels/yield-os \
	tests/alu-tests \
	tests/am-tests \
	tests/cpu-tests

# 最小 TRM 程序已经完成 L4 base profile 构建验证。
PROGRAM_kernels/hello_PATH := kernels/hello
PROGRAM_kernels/hello_STATUS := supported
PROGRAM_kernels/hello_CORE_FEATURES := rv32e
PROGRAM_kernels/hello_DEVICE_FEATURES := uart-tx
PROGRAM_kernels/hello_BOARDS := starrysky-l4
PROGRAM_kernels/hello_VERIFY := build
PROGRAM_kernels/hello_REASON := 已通过 RV32E/ilp32e 构建验证，仍需实际板卡 boot/hardware 验证

# Timer0 已完成独立板级验证；以下基准程序按 build 等级开放。
PROGRAM_benchmarks/coremark_PATH := benchmarks/coremark
PROGRAM_benchmarks/coremark_STATUS := supported
PROGRAM_benchmarks/coremark_CORE_FEATURES := rv32e
PROGRAM_benchmarks/coremark_DEVICE_FEATURES := timer uart-tx
PROGRAM_benchmarks/coremark_BOARDS := starrysky-l4
PROGRAM_benchmarks/coremark_VERIFY := build
PROGRAM_benchmarks/coremark_REASON := 已通过 RV32E/ilp32e 构建验证，尚未完成 benchmark 板上结果验证

PROGRAM_benchmarks/dhrystone_PATH := benchmarks/dhrystone
PROGRAM_benchmarks/dhrystone_STATUS := supported
PROGRAM_benchmarks/dhrystone_CORE_FEATURES := rv32e
PROGRAM_benchmarks/dhrystone_DEVICE_FEATURES := timer uart-tx
PROGRAM_benchmarks/dhrystone_BOARDS := starrysky-l4
PROGRAM_benchmarks/dhrystone_VERIFY := build
PROGRAM_benchmarks/dhrystone_REASON := 已通过 RV32E/ilp32e 构建验证，尚未完成 benchmark 板上结果验证

PROGRAM_benchmarks/microbench_PATH := benchmarks/microbench
PROGRAM_benchmarks/microbench_STATUS := supported
PROGRAM_benchmarks/microbench_CORE_FEATURES := rv32e
PROGRAM_benchmarks/microbench_DEVICE_FEATURES := timer uart-tx
PROGRAM_benchmarks/microbench_BOARDS := starrysky-l4
PROGRAM_benchmarks/microbench_VERIFY := build
PROGRAM_benchmarks/microbench_REASON := 已通过 RV32E/ilp32e C/C++ 构建验证，尚未完成 benchmark 板上结果验证

PROGRAM_kernels/blockchain_PATH := kernels/blockchain
PROGRAM_kernels/blockchain_STATUS := conditional
PROGRAM_kernels/blockchain_CORE_FEATURES := rv32e
PROGRAM_kernels/blockchain_DEVICE_FEATURES := timer
PROGRAM_kernels/blockchain_BOARDS := starrysky-l4
PROGRAM_kernels/blockchain_VERIFY := none
PROGRAM_kernels/blockchain_REASON := 尚未验证 C++ runtime 和 64 位软件运算

PROGRAM_tests/alu-tests_PATH := tests/alu-tests
PROGRAM_tests/alu-tests_STATUS := conditional
PROGRAM_tests/alu-tests_CORE_FEATURES := rv32e alu-tests-verified
PROGRAM_tests/alu-tests_DEVICE_FEATURES := uart-tx
PROGRAM_tests/alu-tests_BOARDS := starrysky-l4
PROGRAM_tests/alu-tests_VERIFY := none
PROGRAM_tests/alu-tests_REASON := 随机 ALU 用例尚未逐核心验证

PROGRAM_tests/cpu-tests_PATH := tests/cpu-tests
PROGRAM_tests/cpu-tests_STATUS := conditional
PROGRAM_tests/cpu-tests_CORE_FEATURES := rv32e cpu-tests-verified
PROGRAM_tests/cpu-tests_DEVICE_FEATURES := uart-tx
PROGRAM_tests/cpu-tests_BOARDS := starrysky-l4
PROGRAM_tests/cpu-tests_VERIFY := none
PROGRAM_tests/cpu-tests_REASON := 必须使用 TEST 逐项审核，当前没有已授权测试项

# CTE 程序必须由经过审核的 Zicsr profile 开放。
PROGRAM_kernels/yield-os_PATH := kernels/yield-os
PROGRAM_kernels/yield-os_STATUS := conditional
PROGRAM_kernels/yield-os_CORE_FEATURES := zicsr ecall mret cte
PROGRAM_kernels/yield-os_DEVICE_FEATURES := uart-tx
PROGRAM_kernels/yield-os_BOARDS := starrysky-l4
PROGRAM_kernels/yield-os_VERIFY := none
PROGRAM_kernels/yield-os_REASON := 需要已完成 CSR smoke 和板测的 CTE profile

PROGRAM_kernels/thread-os_PATH := kernels/thread-os
PROGRAM_kernels/thread-os_STATUS := conditional
PROGRAM_kernels/thread-os_CORE_FEATURES := zicsr ecall mret cte
PROGRAM_kernels/thread-os_DEVICE_FEATURES := timer uart-tx
PROGRAM_kernels/thread-os_BOARDS := starrysky-l4
PROGRAM_kernels/thread-os_VERIFY := none
PROGRAM_kernels/thread-os_REASON := 需要已完成 Context、ECALL、MRET 和板测的 CTE profile

# 图形、输入和资源程序等待真实设备信息与板测。
PROGRAM_kernels/demo_PATH := kernels/demo
PROGRAM_kernels/demo_STATUS := conditional
PROGRAM_kernels/demo_CORE_FEATURES := rv32e
PROGRAM_kernels/demo_DEVICE_FEATURES := gpu
PROGRAM_kernels/demo_BOARDS := starrysky-l4
PROGRAM_kernels/demo_VERIFY := none
PROGRAM_kernels/demo_REASON := L4 显示型号、方向、分辨率和 QSPI CS 尚未确认

PROGRAM_kernels/litenes_PATH := kernels/litenes
PROGRAM_kernels/litenes_STATUS := conditional
PROGRAM_kernels/litenes_CORE_FEATURES := rv32e
PROGRAM_kernels/litenes_DEVICE_FEATURES := gpu input timer
PROGRAM_kernels/litenes_BOARDS := starrysky-l4
PROGRAM_kernels/litenes_VERIFY := none
PROGRAM_kernels/litenes_REASON := 需要 GPU、PS2 输入和计算能力板测

PROGRAM_kernels/slider_PATH := kernels/slider
PROGRAM_kernels/slider_STATUS := conditional
PROGRAM_kernels/slider_CORE_FEATURES := rv32e
PROGRAM_kernels/slider_DEVICE_FEATURES := gpu flash
PROGRAM_kernels/slider_BOARDS := starrysky-l4
PROGRAM_kernels/slider_VERIFY := none
PROGRAM_kernels/slider_REASON := 400x300 资源、裁剪和 Flash/PSRAM 容量尚未验证

PROGRAM_kernels/snake_PATH := kernels/snake
PROGRAM_kernels/snake_STATUS := conditional
PROGRAM_kernels/snake_CORE_FEATURES := rv32e
PROGRAM_kernels/snake_DEVICE_FEATURES := gpu input timer
PROGRAM_kernels/snake_BOARDS := starrysky-l4
PROGRAM_kernels/snake_VERIFY := none
PROGRAM_kernels/snake_REASON := 需要 GPU 和 PS2 输入后端板测

PROGRAM_kernels/typing-game_PATH := kernels/typing-game
PROGRAM_kernels/typing-game_STATUS := conditional
PROGRAM_kernels/typing-game_CORE_FEATURES := rv32e
PROGRAM_kernels/typing-game_DEVICE_FEATURES := gpu input timer
PROGRAM_kernels/typing-game_BOARDS := starrysky-l4
PROGRAM_kernels/typing-game_VERIFY := none
PROGRAM_kernels/typing-game_REASON := 需要 GPU 和 PS2 输入后端板测

PROGRAM_kernels/bad-apple_PATH := kernels/bad-apple
PROGRAM_kernels/bad-apple_STATUS := conditional
PROGRAM_kernels/bad-apple_CORE_FEATURES := rv32e
PROGRAM_kernels/bad-apple_DEVICE_FEATURES := gpu timer audio-fallback
PROGRAM_kernels/bad-apple_HOST_TOOLS := ffmpeg
PROGRAM_kernels/bad-apple_BOARDS := starrysky-l4
PROGRAM_kernels/bad-apple_VERIFY := none
PROGRAM_kernels/bad-apple_REASON := 需要 ffmpeg、GPU 和经过验证的无音频降级路径

PROGRAM_tests/am-tests_PATH := tests/am-tests
PROGRAM_tests/am-tests_STATUS := conditional
PROGRAM_tests/am-tests_CORE_FEATURES := am-test-selected
PROGRAM_tests/am-tests_DEVICE_FEATURES := uart-tx
PROGRAM_tests/am-tests_BOARDS := starrysky-l4
PROGRAM_tests/am-tests_VERIFY := none
PROGRAM_tests/am-tests_REASON := AM 测试必须按子项能力审核，当前未开放笼统的全部测试

# 永久与 L4 ysyx-am 不兼容的程序。
PROGRAM_kernels/nemu_PATH := kernels/nemu
PROGRAM_kernels/nemu_STATUS := unsupported
PROGRAM_kernels/nemu_CORE_FEATURES :=
PROGRAM_kernels/nemu_DEVICE_FEATURES :=
PROGRAM_kernels/nemu_BOARDS := starrysky-l4
PROGRAM_kernels/nemu_VERIFY := none
PROGRAM_kernels/nemu_REASON := 该程序 Makefile 明确只支持 NEMU 平台
