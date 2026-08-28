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
└── tools/          # Python installer/CLI, toolchain manifests, and build helpers
```

---

### 3. Environment Dependencies and Installation

#### Dependencies
- SDK-pinned xPack GNU RISC-V Embedded GCC 15.2.0-1
  (`riscv-none-elf-gcc`, selected and verified for the host by the installer)
- `make`
- `direnv` (Recommended, for automatically loading environment variables in `testdir`)
- Python 3.9 or newer

#### Quick Installation
Run the Python installer. On GNU/Linux it installs to
`~/.local/share/ecos/sdk/3.0.0` by default together with the pinned toolchain selected
for the current host:
```bash
python3 tools/install.py
```
The installer reads the SDK `3.0.0` identity from `tools/sdk-manifest.json`, copies the
manifest, and registers the installed SDK as active. It detects Bash, Zsh, Fish, or
PowerShell, installs the matching `ecos` completion, and maintains a marked `ECOS SDK`
block in that shell's user startup file. The block adds only the CLI path and completion;
it does not persist `ECOS_SDK_HOME`. Reload the file using the command printed after
installation, or open a new terminal. Inspect the complete plan without writes or network
access first with:
```bash
python3 tools/install.py --dry-run
```

Use `--archive <path>` for an offline toolchain archive, or `--skip-toolchain` to
update only the SDK files. Use `--shell bash|zsh|fish|powershell` to override shell
detection, `--shell-profile <path>` to select a startup file, or `--shell none` to
disable shell configuration. `--prefix <path>` names the parent directory; the installer
appends the version from the SDK manifest. For example, `--prefix ~/ecos-sdks` installs
this release at `~/ecos-sdks/3.0.0`. Use `--registration-name <name>` to choose the registration,
`--no-activate` to preserve the current global selection, or `--force` to redeploy the SDK,
replace its registration, and reinstall the toolchain when enabled. Use
`--replace-registration` or `--force-toolchain` to force only that part. See
`python3 tools/install.py --help` for all options.

The 3.0 package does not copy the 2.x shell commands from the source `bin/`
directory. The version directory's `bin/` contains only generated Python `ecos` launchers;
help and completion expose only commands already migrated to Python. The currently
migrated commands are `sdk`, `project`, `build`, `toolchain`, and `completion`.

Toolchain detection, status inspection, and installation are provided by the Python
`ecos` CLI:
```bash
ecos sdk register /path/to/checkout --name dev --activate
ecos sdk list
ecos sdk current
ecos sdk use 3.0.0
ecos sdk pin 3.0.0 --project /path/to/project
ecos sdk doctor
ecos project create hello --path ~/workspace
ecos toolchain detect
ecos toolchain status
ecos toolchain install
ecos toolchain status --format json
ecos completion bash
ecos completion zsh
ecos completion fish
ecos completion powershell
```

SDK paths use a fixed priority: the command-wide `--sdk` selector, the project pin, the
source checkout containing the current path, the compatibility variable `ECOS_SDK_HOME`,
the registry's active entry, then the CLI entry checkout. An invalid higher-priority
selection is an error and never silently falls back to another version. `ecos sdk
unregister` removes only the registration, not SDK files.

Use `ecos toolchain install --dry-run` to inspect the plan without network or file
writes. Use `--archive <path>` to import an offline official archive.

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
