# Hardware validation and evidence

Start with `<SDK>/docs/sdk-3.0-manual-validation.md`, the actual Board README,
Example source/README and relevant Driver contract. For CH32V103 use the selected
SDK's `docs/ch32v103-windows-test-checklist.md` if present. Do not copy a stale
hardware PASS table into a new board/revision's record.

## Build a checklist from facts

1. Record SDK/commit, exact silicon/package/board revision, OS/Python/tool versions,
   supply/reference voltages, wiring, instrument details, project diff and image hash.
2. Query `project list` for compatible examples; read each initializer and logging
   path. Separate unmodified examples, modified test projects and unsupported work.
3. Start with environment/build, hello/boot and console, then GPIO/timer/interrupts,
   buses and devices. Put destructive tests and watchdogs last. Do not flash a batch
   of unreviewed programs automatically.
4. For each item write prerequisites, pins/electrical limits, command, finite wait,
   expected bytes/count/waveform, actual result, evidence and recovery steps.
5. Label results **PASS / FAIL / BLOCKED / N/A** per subtest. Keep compilation,
   host register models, ELF/CI checks and silicon tests in separate columns.

## Match acceptance to what the program actually exercises

| Feature | Required evidence; insufficient shortcut |
| --- | --- |
| Boot/console | Reset and cold boot, correct baud and TX/RX; not just programmer success |
| IRQ | Repeated entry/return, correct event count, main-loop health, shared/disable/re-register behavior; not just a valid vector ELF |
| Timer/PWM/RTC | Measured frequency/duty/time drift including edges; not an assumed input clock or nonzero counter |
| ADC | Known measured input/reference, monotonic readings/errors; not floating-pin noise |
| Bus/device | Correct data and protocol timing, NACK/error/recovery; not only ACK scanning or ready logs |
| DMA | Completion/error/remaining count and verified destination, then callbacks; not just enable/init |
| Flash/EEPROM | Authorized reserved region, erase/read/write/compare, neighbor preservation and reset/power retention; not a capacity constant |
| Backup | Read retained data before overwriting on reboot; an always-write-then-read example cannot prove retention |
| CRC/ID | Independent matching reference/encoding or exact device identity; repeated results alone are insufficient |
| USB | Defined controller versus endpoint/class/descriptor scope; init is not Windows enumeration |
| Low power | Known wake source and measured current/recovery; status flags alone are insufficient |
| Watchdog | Correct refresh/stop-refresh/window timing and reset-cause flag; not a generic boot loop |

Interrupt handlers set state/counters only; report in the main loop. Any helper
snippet modifying an example must be compiled in `testdir/` and labelled modified.
Do not promise a runtime value based on another Target's clock or package.

## Safety gate before each hardware action

Explain and obtain consent for actual flashing/unprotect/erase, EEPROM writes,
watchdog arming, low-power entry, option-byte changes or USB driver changes. Check
Board flash defaults even for a plain `ecos flash`. Confirm backups and recovery.
Never erase executing code or a data load image; default linker scripts do not
necessarily reserve the proposed test page. Do not alter the published SDK's linker
layout in place to run a destructive experiment.

Without connected hardware/instruments, mark hardware items untested or blocked.
Users can run the supplied steps and return logs; an Agent must not fill in PASS.

## Result template

```text
SDK root / manifest version / commit:
Host OS / Python / toolchain / programmer:
Silicon marking / package / board revision:
Supply and reference voltage / wiring:
Project / Board / Target / profile / source diff:
Test ID and scope (stock / modified / extended):
Exact command / exit / artifact hash:
Build/model/CI result:
Hardware result: PASS / FAIL / BLOCKED / N/A
Expected and actual bytes/count/time/waveform:
Reproduction count / run duration:
Log / capture / fault mcause-mepc-mtval (if available):
Limits / recovery / next step:
```
