# ECOS Embedded SDK

English | [中文](README.md)

**ECOS Embedded SDK** is a production-grade, modular bare-metal firmware development kit tailored for the StarrySky series of RISC-V chips and microcontrollers (such as `StarrySkyC1`, `StarrySkyC2`, `StarrySkyL3`, `StarrySkyL3_1`).

Built around the new **HAL V2** hardware abstraction layer, this SDK utilizes Kconfig for graphical configuration and the CMake/Ninja build system, aiming to provide an efficient and secure embedded development experience.

This is the `v3.1` development line: SDK `3.1.0-dev.0`, CLI package `3.1.0.dev0`.
It is not the final `3.1.0` release. The `3.0.1` branch is maintenance-only, and the
CH32V103 hardware-validation boundaries are unchanged. See
[versioning and release policy](docs/versioning.md).

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
- **Modular Build with Kconfig**: Integrates a Linux kernel-style Kconfig tool. Peripheral drivers and optional components are selected by project requirements; Core Runtime and the minimal libc subset are included by default.
- **Strictly Separated Source Tree**: Provides the Python `ecos project create` tool, allowing users to create independent project directories **anywhere in the OS** for development, eliminating source code pollution caused by editing the SDK source tree directly.
- **XIP (Execute-In-Place) Support**: Supports executing code directly from Flash. Combined with `ld` linker script optimizations for memory mapping, it significantly saves SRAM space.
- **Rich External Device Support**: Natively supports external peripherals, such as ST7735 / ST7789 screen drivers, PCF8563 external RTC clock chips, SGP30 gas sensors, etc.

---

### 2. Directory Structure
```text
ecos/embedded-sdk/
├── boards/          # Board Support Package (BSP), including pin definitions, clock configs, and linker scripts
│   ├── StarrySkyC1/
│   ├── StarrySkyC2/
│   ├── StarrySkyL3/
│   └── StarrySkyL3_1/
├── components/     # Core Runtime, middleware, and static libraries (e.g., libc, libgcc)
├── devices/        # External device driver components (e.g., st7735, st7789, sgp30, pcf8563)
├── hal/            # Core Hardware Abstraction Layer V2 interface definitions (hal_uart, hal_timer, hal_gpio, etc.)
├── examples/       # Official bare-metal applications
└── tools/          # Python installer/CLI, toolchain manifests, and build helpers
```

---

### 3. Environment Dependencies and Installation

#### Dependencies
- SDK-pinned xPack GNU RISC-V Embedded GCC 15.2.0-1
  (`riscv-none-elf-gcc`, selected and verified for the host by the installer)
- CMake 3.20 or newer (the installer pins `cmake==3.31.10` in the SDK)
- Ninja (the installer pins `ninja==1.11.1.4` in the SDK)
- wlink 0.1.3 (selected and verified for the host by the installer; used by `ecos flash`)
- Python 3.9 or newer

#### Quick Installation
Run the Python installer. This development checkout is registered in place; it is not
copied to a version directory. The installer also installs the host CLI, pinned
toolchain, wlink, and CMake/Ninja build dependencies selected for the current host:
```bash
python3 tools/install.py
```
The installer reads the SDK `3.1.0-dev.0` identity from `tools/sdk-manifest.json`
and registers this checkout as the active SDK. It detects Bash, Zsh, Fish, or
PowerShell, installs the matching `ecos` completion, and maintains a marked `ECOS SDK`
block in that shell's user startup file. The block adds the SDK CLI and host build-tool
paths plus completion; it does not persist `ECOS_SDK_HOME`. Reload the file using the
command printed after installation, or open a new terminal. Inspect the complete plan
without writes or network access first with:
```bash
python3 tools/install.py --dry-run
```

Use `--archive <path>` for an offline xPack archive and `--wlink-archive <path>` for an
offline wlink archive; the first Python/CMake/Ninja install still needs PyPI or a populated
pip cache. `--skip-toolchain` skips only the cross compiler and `--skip-wlink` skips only
wlink; the SDK's Python, CMake, and Ninja dependencies are still installed. Use
`--shell bash|zsh|fish|powershell` to override shell
detection, `--shell-profile <path>` to select a startup file, or `--shell none` to
disable shell configuration. For release installations, `--prefix <path>` names the
parent directory and the installer appends the manifest version, for example
`~/ecos-sdks/<sdk_version>`. Development mode keeps using the checkout.
Use `--registration-name <name>` to choose the registration,
`--no-activate` to preserve the current global selection, or `--force` to redeploy the SDK,
replace its registration, and reinstall the toolchain when enabled. Use
`--replace-registration` or `--force-toolchain` to force only that part. See
`python3 tools/install.py --help` for all options.

The installer creates the Python `ecos` launcher in the host entry directory. The SDK source
tree no longer contains the old shell commands; help and completion expose only Python CLI commands. Public commands
cover SDK and toolchain management, projects, resource and schema queries,
validation, configuration, builds, cleanup, diagnostics, flashing, serial monitoring,
and completion. See the [CLI command reference](docs/cli.md) for the full list.

Toolchain detection, status inspection, and installation are provided by the Python
`ecos` CLI:
```bash
ecos sdk register /path/to/checkout --name dev --activate
ecos sdk list
ecos sdk current
ecos sdk use 3.1.0-dev.0
ecos sdk pin 3.1.0-dev.0 --project /path/to/project
ecos sdk doctor
ecos project list
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

SDK 3.1 projects are created and orchestrated by the Python `ecos` CLI. Core builds use
CMake and Ninja. Keep application projects outside the SDK installation and do not edit
the source files under `examples/`.

#### Step 1: Create your independent project directory
Thanks to the `ecos` command and environment variable configuration, you can create your project anywhere on your computer.
```bash
# Assuming you are in your own development workspace
cd ~/my_workspace/
```

#### Step 2: Create the project
Create a project from an SDK Example and select its Board:
```bash
ecos project list --format json
ecos project create hello --name my_smoke_test --path ~/my_workspace \
    --board starrysky-l3-1
cd my_smoke_test
```

#### Step 3: Validate, configure, and build
```bash
ecos validate --project ~/my_workspace/my_smoke_test --format json
ecos configure --project ~/my_workspace/my_smoke_test
ecos build --project ~/my_workspace/my_smoke_test --format json
```
Build outputs and `artifacts.json` are written to the project's `build/` directory. After
changing the Board metadata, run `ecos configure` and `ecos build` again; application
sources remain shared across compatible Boards.

Hardware operations use the same CLI:

```bash
ecos flash --project ~/my_workspace/my_smoke_test
ecos monitor --project ~/my_workspace/my_smoke_test --port /dev/ttyUSB0
```

#### Optional Agent Skill

The SDK includes a portable [ECOS SDK Agent Skill](.agents/skills/ecos-sdk/SKILL.md)
for project development, porting, troubleshooting and evidence-based hardware
validation. It queries the selected SDK instead of embedding a Board capability
list. Installation into an external application or user skills directory is opt-in:

```bash
python3 tools/install_skill.py --project /path/to/app --dry-run
python3 tools/install_skill.py --project /path/to/app
```

The normal SDK installer does not enable it or edit Agent settings. See
[docs/agent-skill.md](docs/agent-skill.md) for safe updates, client discovery,
Windows usage and the optional read-only project checker.

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

#### ECOS Core Runtime (Errors and Logging)
Core Runtime and the minimal libc subset are included by default in SDK 3.x projects. Core Runtime provides shared `ECOS_ERR_*` error semantics and a heap-free logging service; libc supplies the memory operations needed by the freestanding compiler. A successfully initialized BSP Console installs itself as the log writer; output defaults to ASCII with CRLF line endings and no ANSI color.

- **Code Usage Example**:
  ```c
  #include "ecos/bsp/console.h"
  #include "ecos/log.h"

  int main(void) {
      ECOS_RETURN_ON_ERROR(bsp_console_init());
      ecos_log_set_level(ECOS_LOG_DEBUG);
      ECOS_LOGI("app", "Device booted successfully");
      ECOS_LOGW("app", "Abnormal input detected");
      ECOS_LOG_ERR("app", ECOS_ERR_IO, "initialize peripheral");
      return ECOS_OK;
  }
  ```

  Use `ECOS_RETURN_ON_ERROR()` to propagate an error to the caller,
  `ECOS_GOTO_ON_ERROR()` when common cleanup is required, and
  `ECOS_PANIC_ON_ERROR()` to report an unrecoverable application error before
  halting. Each macro evaluates its result expression exactly once.

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
