# ECOS build and Windows troubleshooting

Use the selected SDK's `docs/cli.md`, `docs/install.md`, public headers and the
Board README. Query before repairing. Never download/reinstall tools, rewrite SDK
pins, replace registered releases or rebind a USB driver without permission.

## Capture the minimum evidence

Record SDK root/manifest version/channel/commit, project/Board/Target/profile,
host OS/Python, exact failing command/exit code, complete diagnostics and first
compiler/linker error. Keep the project source/config diff and artifact hashes.
Redact secrets and unrelated host information before sharing logs.

```text
ecos sdk current --project <PROJECT> --format json
ecos --sdk <SDK_ROOT> project info --project <PROJECT> --format json
ecos --sdk <SDK_ROOT> validate --project <PROJECT> --format json
ecos --sdk <SDK_ROOT> toolchain status --format json
ecos --sdk <SDK_ROOT> doctor --project <PROJECT> --format json
```

The optional skill `scripts/check_project.py` collects the first four as JSON. It
is read-only and does not build. `doctor` may inspect connected devices; run it
separately when that is appropriate. Preserve failure payloads; do not replace a
failed command with an empty successful report.

## Diagnose by layer

| Symptom | First checks |
| --- | --- |
| SDK/identity error | Selected root versus project pin, manifest SDK ID/version, CLI compatibility; no silent fallback |
| Capability mismatch | Effective Example/Component requirements versus Board resources and Target capability; do not bypass validation |
| Missing header/symbol | Component dependency graph, include visibility and selected Target capability sources; no implicit declarations |
| ISA/ABI mismatch | Target `march/abi`, SDK toolchain pin, actual compiler and compile commands; no random flags copied from another MCU |
| Memory overflow/link failure | Current linker script, `.text/.rodata`, `.data` load image, SRAM/BSS/stack and runtime helpers |
| Incremental build broken | Persistent config, generated fingerprints and CMake language initialization; clean only recognized project outputs |
| Artifact rejected by flash | Rebuild after source/config changes; verify hashes/ISA/Board; never edit the artifact manifest to bypass checks |
| Downloaded but no output | Flash provider/address, BOOT selection, reset/run state, console TX/RX, common ground, baud, terminal timing |
| First interrupt freezes | Actual IRQ map/controller, mtvec/vector encoding, ABI/context, status clearing and allocation ownership |
| I2C/SPI/ADC wrong data | Electrical levels/pulls, exact pins, bus clock, sample/transfer timing and device protocol; use measurements |

An API/driver default may use a legacy reference clock instead of Board values.
Read the example initializer: a declared resource does not prove the app consumed
it. Timers, PWM, RTC/LSI and USB clock errors need actual time/waveform evidence.

## Windows / CH32 checks

Only apply these after querying that the project really selects CH32V103. Read
`<SDK>/boards/CH32V103C8T6/README.md` and, if available,
`<SDK>/docs/ch32v103-windows-test-checklist.md` for the selected revision's limits.

- Refreshing SDK source JSON does not refresh an old installed CLI's bundled
  manifests. Offer the current install guide's refresh command; ask before running.
- Windows x64 host identity does not imply x64 wlink executable. The current CH32
  baseline uses x86 wlink for common WCH WDM drivers. Read the current tool pin;
  do not hard-code a different release into the skill.
- `ECOS_FLASH_DRIVER_INCOMPATIBLE` is not "board missing". Do not blindly use
  Zadig, swap every USB interface to WinUSB or overwrite the serial interface.
- An explicit `flash --tool` or `ECOS_WLINK` can override a managed wlink; merely
  adding another executable to PATH may not. Check actual selection first.
- Separate WCH-Link debug USB, USB-UART and native MCU USB. Native USB controller
  initialization without descriptors/endpoints cannot produce a working CDC port.
- For one-shot boot logs open the serial capture **before reset**. The current
  `ecos monitor` reads output but does not forward keyboard input; use a bidirectional
  terminal for an interactive shell, with only one process owning the COM port.
- In PowerShell use quoted paths/argument arrays, check `$LASTEXITCODE` immediately
  and use `python` where `python3` is unavailable. Do not translate commands into
  `bash`, `make menuconfig` or persistent global environment changes.

Change one layer at a time and retain a known-good hello/recovery image. Report
what was demonstrated and what remains a hypothesis; never label an unmeasured
clock, unconnected sensor or arbitrary register value as verified.
