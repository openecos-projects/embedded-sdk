# ECOS Embedded SDK

English | [中文](README.md)

**ECOS Embedded SDK** is a production-grade, modular bare-metal firmware development kit tailored for the StarrySky series of RISC-V chips and microcontrollers (such as `StarrySkyC1`, `StarrySkyC2`, `StarrySkyL3`, `StarrySkyL3_1`).

Built around the new **HAL V2** hardware abstraction layer, this SDK utilizes Kconfig for graphical configuration and the Make build system, aiming to provide an efficient and secure embedded development experience.

---

## Table of Contents
1. [Core Architecture and Features](#1-core-architecture-and-features)
2. [Directory Structure](#2-directory-structure)
3. [Environment Dependencies and Installation](#3-environment-dependencies-and-installation)
4. [Standard Development and Testing Workflow (Must Read)](#4-standard-development-and-testing-workflow-must-read)
5. [HAL V2 API Migration Guide](#5-hal-v2-api-migration-guide)
6. [Third-Party Component Support](#6-third-party-component-support)
7. [Acknowledgements](#7-acknowledgements)

---

### 1. Core Architecture and Features
- **HAL V2 Hardware Abstraction Layer**: Abandons legacy APIs that directly read/write low-level registers (e.g., `REG_UART_0_RX`) and use magic number configurations. It adopts a comprehensive set of standardized `hal_*` interfaces, supporting unified GPIO MUX (multiplexing) and FCFG (function) configurations.
- **Modular Build with Kconfig**: Integrates a Linux kernel-style Kconfig tool. It supports on-demand pruning of peripheral drivers (I2C, SPI, RTC, etc.) and system components (libc, libgcc, TimmoLog, etc.), automatically generating configuration macros.
- **Strictly Separated Source Tree**: Provides the `ecos init_project` tool, allowing users to create independent project directories **anywhere in the OS** for development, eliminating source code pollution caused by directly modifying the SDK's `templates/`. (SDK developers can use the dedicated `testdir/` directory for internal rapid testing and verification).
- **XIP (Execute-In-Place) Support**: Supports executing code directly from Flash. Combined with `ld` linker script optimizations for memory mapping, it significantly saves SRAM space.
- **Rich External Device Support**: Natively supports external peripherals, such as ST7735 / ST7789 screen drivers, PCF8563 external RTC clock chips, SGP30 gas sensors, etc.

---

### 2. Directory Structure
```text
ecos/embedded-sdk/
├── board/          # Board Support Package (BSP), including pin definitions, clock configs, and linker scripts
│   ├── StarrySkyC1/
│   ├── StarrySkyC2/
│   ├── StarrySkyL3/
│   └── StarrySkyL3_1/
├── components/     # System-level middleware and static libraries (e.g., libc, libgcc, TimmoLog)
├── devices/        # External device driver components (e.g., st7735, st7789, sgp30, pcf8563)
├── hal/            # Core Hardware Abstraction Layer V2 interface definitions (hal_uart, hal_timer, hal_gpio, etc.)
├── scripts/        # Make build scripts and rules
├── templates/      # Official basic/peripheral bare-metal example templates
├── testdir/        # (For SDK developers only) Internal dedicated environment for rapid template testing
└── tools/          # Build auxiliary tools (kconfig, fixdep, ecos CLI scaffolding)
```

---

### 3. Environment Dependencies and Installation

#### Dependencies
- RISC-V cross-compilation toolchain (`riscv64-unknown-elf-gcc` / `riscv32-unknown-elf-gcc`)
- `make`
- `direnv` (Recommended, for automatically loading environment variables in `testdir`)
- Python 3 (Optional, for some build script automations)

#### Quick Installation
Run the installation script in the root directory to configure the basic environment:
```bash
./install.sh
```
After the script finishes, you need to reopen the terminal or run `source ~/.bashrc` to update the environment variables.

---

### 4. Standard Development and Testing Workflow (Must Read)

> ⚠️ **WARNING (CRITICAL RULES)**:
> It is absolutely forbidden to directly modify and execute `make` builds within the `templates/` directory in the SDK installation path! All development and testing MUST be done in a newly created project directory outside the SDK to ensure the original template code remains unpolluted.

#### Step 1: Create your independent project directory
Thanks to the `ecos` command and environment variable configuration, you can create your project anywhere on your computer.
```bash
# Assuming you are in your own development workspace
cd ~/my_workspace/
```

#### Step 2: Initialize the project
Use the `ecos` scaffolding tool to pull code from the official template and specify the target board (e.g., `l3_1`):
```bash
# Format: ecos init_project <template_name> -name <project_name> -target <board_model>
ecos init_project smoke_test -name my_smoke_test -target l3_1
cd my_smoke_test
```

*(Note: If you are a kernel maintainer participating in the development of this SDK, you can directly use `direnv` in the `testdir/` directory for rapid internal template debugging.)*

#### Step 3: Configure the System (Kconfig)
In the example project directory, you can choose graphical configuration or headless default configuration:

- **Interactive Graphical Configuration (Recommended for daily developers)**:
  ```bash
  make menuconfig
  ```
  Check the driver modules, configuration optimization levels, or system libraries you need in the interface. A configuration file will be generated after saving and exiting.

- **Headless Default Configuration (Recommended for CI/Scripts/AI Agent automated testing)**:
  If you do not need to manually change the menu, you can directly use the underlying conf tool to silently generate the configuration:
  ```bash
  $ECOS_SDK_HOME/tools/kconfig/build/conf --alldefconfig Kconfig
  $ECOS_SDK_HOME/tools/kconfig/build/conf --syncconfig Kconfig
  ```

#### Step 4: Compile Firmware
```bash
make -j$(nproc)
```
After successful compilation, `retrosoc_fw.elf` (ELF debug file), `retrosoc_fw.hex`, and `retrosoc_fw.bin` (binary flashing file) will be generated in the `build/` directory, and the current Flash/MEM memory usage report will be output. Use the `.bin` or `.hex` file for flashing.

---

### 5. HAL V2 API Migration Guide
The SDK has completely deprecated the low-level register direct operation code from the legacy (C1/C2) era. When writing new code or migrating old projects, please strictly follow these interface mapping relationships:

- **UART Initialization**: `sys_uart_init()` -> `hal_sys_uart_init()`
- **I2C Bus**: `i2c_init(...)` -> `hal_i2c_init(...)`
- **Timer and Delay**: Deprecated hardware infinite loop `delay_s() / delay_ms()`, use hardware Timer based `hal_delay_ms(timer_id, ms)`.
- **System Tick Clock**: Deprecated `sys_tick_init()`, use standard `hal_sys_tick_init(0)` and `hal_get_sys_tick(0)`.
- **GPIO Pin Configuration**: No longer use bitmask splicing to directly write to `PADDIR` / `PADOUT`, you must use the encapsulated `gpio_hal_set_mux(port, pin, func)` and `gpio_hal_set_fcfg(port, pin, mode)` to set the pin's function and multiplexing mode.

---

### 6. Third-Party Component Support

#### TimmoLog (Smart Logging System)
This SDK deeply integrates [TimmoLog](https://github.com/XHTimmo/TimmoLog), an advanced logging system supporting ANSI colors, and has specially mapped `printf` serial output for the RISC-V Bare-metal environment, making it very suitable for developers and AI to read structured logs.

- **How to enable**: Turn on the `TimmoLog Support` option in `Build Configuration -> Library Configuration` of `make menuconfig`.
- **Code Usage Example**:
  ```c
  #include "log.h"

  int main() {
      hal_sys_uart_init();
      
      // Initialize the logging system, set the minimum print level to DEBUG
      log_init(LOG_DEBUG, NULL);
      
      // Print logs with level prefixes, time, and ANSI colors
      log_info("[SYSTEM] Device booted successfully");
      log_warn("Abnormal input detected, automatically skipping...");
      log_error("Hardware initialization failed!");
      
      return 0;
  }
  ```

---

### 7. Acknowledgements
Thanks to the following developers for their code contributions to the ECOS Embedded SDK:

- [XHTimmo](https://github.com/XHTimmo)
- [maksyuki](https://github.com/maksyuki)
- [Krismile233](https://github.com/Krismile233)
- [FINALxxx](https://github.com/FINALxxx)
- 雪泥喵爪
- Ayana nana
- [myyerrol](https://github.com/myyerrol)
