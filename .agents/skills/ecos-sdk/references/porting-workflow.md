# Target / Board / peripheral / Device porting

Use `<SDK>/docs/schemas.md`, `docs/api/README.md` and current Component/Target/Board
manifests. Read the relevant HAL and Driver headers in full before implementing.
Do not assume the installed skill and the selected SDK are the same revision.

## 1. Scope and hardware evidence

Record exact MCU/SoC, package, board revision, memory, oscillator/reference clocks,
reset behavior, pin mux/remaps, peripheral instances and IRQ numbers. Cite the
reference manual/datasheet revision or vendor source commit used. Shared vendor
headers often define IP absent on a particular part: do not claim CAN, DMA2, a
second ADC or GPIO pins simply because a macro exists.

Classify what is missing: public contract, Target backend, Board wiring/resource,
Device protocol, example integration or hardware validation. Reuse an existing
public HAL only if its semantics match. Do not disguise standard SPI as a private
QSPI, PFIC as PLIC, or a free-running counter as a calendar clock.

## 2. Layer and dependency changes

| Layer | Responsibility |
| --- | --- |
| `hal/<module>/include/ecos/hal/` | Internal hardware contract; units, lifecycle, errors and unsupported operations |
| `components/soc/<target>/hal/` | Register access, clock/reset, silicon-specific implementation |
| `drivers/<module>/` | Public peripheral API and validation/error propagation |
| `devices/<device>/` | External device protocol over public Drivers |
| `boards/<board>/` | Board-to-Target mapping, physical resources and BSP |
| `examples/` | Portable application using declared dependencies/resources |

For a new backend update `ecos-soc.yml` capabilities, `capability_sources` and
`capability_include_dirs`. Expose only implemented semantics. Add Board resources
only for known wiring and extend the **current** schema/parser/generator if a new
resource requires typed macros. Do not add arbitrary unknown manifest fields.

Each Component needs `ecos-component.yml` with real sources/include dirs/dependencies
and capability requirements. Public headers must not accidentally depend on private
include paths. Avoid global enum changes solely to make unavailable instances seem
supported. Check all existing Targets affected by contract changes.

## 3. Register and lifecycle review

- Confirm base/offset, access width and reserved gaps independently; note W1C,
  W0C and read-to-clear semantics. Verify read/modify/write is legal.
- Verify GPIO electrical mode, clock source/divisors, bus versus timer frequency,
  software trigger, startup/calibration delays and bounded ready waits.
- Validate alignment, ranges, overflow, zero/max periods, duty 0/100%, transfer
  length 1/2/3/N, ACK/NACK/STOP/repeated START and partial/error recovery.
- Track ownership per peripheral/channel/IRQ. Define init/reinit/start/stop/deinit
  behavior, busy conflicts and cleanup after failed allocation.
- Use the Target's actual interrupt controller and toolchain ABI. Verify vector
  alignment/slot layout, saved context, ISR flag clearing, shared IRQ dispatch,
  callback lifetime and concurrency; do not assume vendor fast ABI availability.
- DMA needs actual request routing, buffer lifetime, memory ranges/cache/fences,
  remaining count, error/completion handling and channel ownership. Merely writing
  enable or seeing an init log is not a transfer test.
- Document genuinely unsupported operations. Do not silently ignore configuration
  fields to simulate portability; if an existing backend does, surface that limit.

## 4. Testing and integration gates

Create independent projects under `<SDK>/testdir/<TASK>/`, never build Example or
Target source directories directly. Use `ecos validate/configure/build` with an
explicit SDK and no interactive menuconfig. Test both direct HAL/Driver logic and
real dependency/source selection, not just one hand-linked file.

1. Parameter/error/lifecycle tests, including retry and resource conflicts.
2. Host register model where useful; model W1C/W0C and event sequencing, not only
   constants. Label it as a model, not silicon evidence.
3. Cross compile/link the example; inspect compile commands, ELF/vector layout,
   MAP, memory bounds and generated artifacts. Reconfigure/incremental build too.
4. Discover compatible examples via `project list`; verify changed contracts on
   other Targets. Check the existing CI matrix before adding duplicate static lists.
5. Add a hardware checklist with wiring, instrumentation, measurable criteria and
   rollback for destructive tests. Confirm logging actually tests the feature:
   Flash info is not erase/write, USB controller init is not enumeration.

A complete change pairs backend, capability, Board resource (when applicable),
example, tests, public docs and hardware steps. Do not change released tags/branches
or include unrelated local documents. Commit/push only when explicitly requested.
