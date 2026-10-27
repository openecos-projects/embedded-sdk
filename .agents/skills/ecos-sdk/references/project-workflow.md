# Application project workflow

Read paths below from the **selected SDK root**, not the skill directory.
Start with `<SDK>/docs/cli.md`, `docs/schemas.md`, `docs/api/README.md` and the
relevant module's public header in `drivers/`, `devices/` or `boards/include/`.

## 1. Inspect the selection

Replace angle-bracket placeholders; quote filesystem paths for the current shell.
These are existing CLI commands, not shell functions:

```text
ecos sdk current --project <PROJECT> --format json
ecos --sdk <SDK_ROOT> board list --format json
ecos --sdk <SDK_ROOT> board describe <BOARD_ID> --format json
ecos --sdk <SDK_ROOT> target describe <TARGET_ID> --format json
ecos --sdk <SDK_ROOT> project list --format json
ecos --sdk <SDK_ROOT> driver describe <DRIVER_ID> --format json
```

If no project exists, pass the intended workspace to `sdk current`. Confirm the
resolved root with the user if multiple SDKs/pins are plausible. For existing
projects read `.ecos/project.yml` and any `.ecos/sdk.json` pin. An explicit SDK
selection must agree with the intended project identity; do not rewrite the pin
or SDK version just to make validation pass.

Use `project list`'s effective requirements and `supported_boards`. Do not infer
availability from a similar board name, generic UART port count or example README.
Target-only projects have no implicit console, pin mapping or flash provider.

## 2. Create a separate project

Choose the closest compatible Example; `hello` is the minimal starting point.
Ask for the exact Board if not specified. SDK development tests use
`<SDK>/testdir/<TASK>/`; normal user applications live outside the SDK installation.

```text
ecos --sdk <SDK_ROOT> project create <EXAMPLE> --name <NAME> --path <PARENT> --board <BOARD_ID> --dry-run --format json
ecos --sdk <SDK_ROOT> project create <EXAMPLE> --name <NAME> --path <PARENT> --board <BOARD_ID>
```

`--path` names the **parent**; creation appends `<NAME>`. An Example is its global
manifest name, not `peripherals/...`. Do not use `--force` on an existing project
without explicit replacement permission. To switch selections, preview
`project set-board/set-target/set-profile --dry-run` and preserve application changes.

## 3. Implement through public interfaces

- Read `ecos-example.yml`: add dependencies using actual Component IDs. Read the
  selected `ecos-component.yml` dependencies, `requires` and exported include dirs.
- Use BSP for board services, Device Drivers for devices, Drivers for peripherals;
  include `ecos/board_resources.h` when generated resources are available.
- Do not copy a Target implementation into an application. Standard SPI is not the
  same contract as a Target's specialized QSPI. Read both headers before choosing.
- Initialize buses before attached Device Drivers. Verify controller/channel IDs,
  physical pins, electrical levels, clock units, reference clocks and chip-select
  ownership. Defaults can be SoC-specific and may not consume Board resources.
- Interpret `ecos_err_t` and result counts correctly: negative is failure, zero is
  ordinary success, positive may carry bytes/status. A short successful UART write
  is not a complete transaction. Read deinit/irreversible-enable limitations.
- New app code must not include `ecos/hal/*`, `*_soc.h`, legacy `board.h` or SDK 2.x
  `hal_*.h` compatibility headers. For migration use current public examples, not
  obsolete symbol mappings in legacy documentation.

## 4. Validate and build headlessly

```text
ecos --sdk <SDK_ROOT> validate --project <PROJECT> --format json
ecos --sdk <SDK_ROOT> configure --project <PROJECT>
ecos --sdk <SDK_ROOT> toolchain status --format json
ecos --sdk <SDK_ROOT> build --project <PROJECT> --format json
```

Read JSON `status`, `diagnostics` and the process exit code; a toolchain status
command can succeed while its installation state is not `installed`. Check
`validate`'s resolved graph to verify the intended backend was selected. Do not
edit `.ecos/generated/`, `build/`, cached absolute paths or `artifacts.json` to
repair a source/manifest error. Persistent Kconfig choices go in the project's
`.ecos/project.config`, followed by configure/build. No interactive menuconfig.

Eliminate all implicit function declarations, include/type mismatches, unresolved
symbols and regressions. Inspect warnings rather than suppressing them globally.
When a failure needs a clean build, preview `ecos clean --dry-run`; do not delete
unknown user directories with `rm -rf` or equivalent.

## 5. Inspect artifacts, then request hardware authorization

Check `build/artifacts.json`, ELF/MAP/size, ISA/ABI, memory limits, source/config
fingerprints and hashes. Do not guess a firmware filename or download address.
Use the Board's flash provider after the user authorizes it; inspect erase/unprotect
defaults and distinguish BIN (no address) from HEX (addressed records).
Read [hardware-validation.md](hardware-validation.md) before declaring runtime
success. Supply an exact wiring plan and a bounded capture command, not an
unattended long-running monitor.
