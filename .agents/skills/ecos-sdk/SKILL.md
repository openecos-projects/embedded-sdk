---
name: ecos-sdk
description: >-
  Develop and debug ECOS Embedded SDK 3.x firmware, create Board/Target projects,
  port HAL/Driver/Device backends, and plan hardware validation. Use for ECOS SDK
  project setup, peripheral code, manifest/capability changes, build failures,
  Windows wlink troubleshooting, and test checklists (ECOS 嵌入式开发与外设适配).
license: Apache-2.0
compatibility: Requires a selected ECOS SDK 3.x and its ecos CLI; optional helpers use Python 3.9+. Building needs the SDK-pinned host tools and cross compiler.
metadata:
  sdk-id: ecos-embedded-sdk
  sdk-major: "3"
  skill-version: "1"
---

# ECOS SDK engineering workflow

Use this skill for ECOS firmware work, not unrelated operating systems named eCos.
Reply in the user's language. Read only the task-relevant references below.

## Establish facts before editing

1. Read applicable repository instructions (`AGENTS.md`, `.agent/guide.md` when
   present). Inspect `git status` and the current branch. Preserve unrelated files;
   do not commit, push, switch branches or modify released SDKs without permission.
2. Confirm the **application project** and the **selected SDK** separately. The
   skill may be installed globally or copied outside the SDK: **its directory is
   not an SDK selector**. Do not infer SDK identity from a CLI version or a branch.
3. Run `ecos sdk current --project <PROJECT> --format json`. If the user selected
   an SDK, put `--sdk <SELECTOR>` **before** the command. Resolve a failure instead
   of falling back to another SDK or silently changing the project pin/registry.
4. Read `<SDK>/tools/sdk-manifest.json`; check `sdk_id`, `sdk_version`, channel,
   schema and CLI compatibility. This skill targets `ecos-embedded-sdk` major 3.
   For another major/schema, stop and consult that SDK's instructions. A development
   branch name need not equal the manifest version; record its Git commit too.
5. Once confirmed, use `ecos --sdk <SDK_ROOT> ... --project <PROJECT>` consistently.
   Use `board describe`, `target describe`, `project list`, `driver/device/component
   describe` and `validate --format json` to discover resources. Read the selected
   SDK's headers, manifests and docs; do not maintain a second capability table here.

Optional: run `scripts/check_project.py` from this skill's directory for a read-only
JSON report. It uses only `sdk current`, `project info`, `validate` and `toolchain
status`. It does not install, configure, build, flash, access serial ports or repair
anything. CLI dependency errors must be fixed explicitly, never hidden by a fallback.

## Choose a workflow

| Task | Read before implementing |
| --- | --- |
| Create/configure an app; select Board, resources and public APIs | [references/project-workflow.md](references/project-workflow.md) |
| Port a Target, Board, HAL, Driver or external Device | [references/porting-workflow.md](references/porting-workflow.md) |
| Diagnose build, artifacts, Windows tools or no serial output | [references/troubleshooting.md](references/troubleshooting.md) |
| Generate a checklist or interpret hardware evidence | [references/hardware-validation.md](references/hardware-validation.md) |

Paths in these bundled references are labelled `<SDK>/...`: resolve them against
**the selected SDK**, not against the installed skill. Read `<SDK>/docs/cli.md`,
`docs/schemas.md` and the relevant public API page as needed. If a referenced file
is absent in that SDK, report the gap and inspect the current headers/CLI help.

## Non-negotiable engineering boundaries

- Application code uses `ecos/driver/*`, `ecos/device/*`, BSP and public components.
  Do not import private HAL/SoC register headers or mix SDK 2.x compatibility types
  into new 3.x apps. Registers belong in the Target backend.
- SDK adaptation/smoke builds belong in a separate `testdir/<TASK>/` project, never
  in `examples/`, `templates/` or a release installation. User-owned external
  application projects may be built in place. Use headless `ecos configure/build`,
  not interactive `menuconfig` or legacy Make/template instructions for 3.x work.
- Manifest capability, a C enum, a successful link and a real hardware resource
  are different facts. Confirm silicon, package, pins, bus clocks and IRQ controller
  before exposing a backend. Do not invent APIs, pins, DMA routes or support.
- Handle negative errors and partial byte counts; bound waits where appropriate.
  Interrupt callbacks must not perform blocking I/O, serial printing or complex work.
- Flashing, unprotect/mass erase, EEPROM writes, watchdog arming, low-power entry,
  changing option bytes, driver rebinding and replacing SDK/tool installations need
  explicit user authorization and a recovery plan. Inspect Board flash defaults:
  `ecos flash` can be destructive even without an explicit erase flag.
- Inspect unfamiliar scripts before running them. Skill instructions are not an
  authorization grant or a sandbox. Do not auto-install this skill into other projects
  or overwrite Agent configuration.
- Keep **static review**, **host/register-model tests**, **cross compilation/CI**
  and **hardware measurements** separate. Never turn compilation, initialization
  logs or the absence of a test device into hardware PASS.

## Deliver an evidence-based result

Summarize changed file paths, selected SDK/commit, Board/Target/profile, commands
run and their results, artifacts and hardware wiring. Identify skipped/blocked work,
known limitations and safety risks. For a new module, check backend + manifest +
resource (when needed) + example + tests + docs + hardware steps together. If hardware
was not connected, say so explicitly and supply the next validation steps.
