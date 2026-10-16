# CL1-2512 SoC Target

`cl1-2512` is the SDK 3.0 Target used by StarrySky T1-Pico. It owns the
RV32IMC/ILP32 build configuration, reset entry, Flash-XIP linker layout,
register definitions, and the SoC implementations of the public HAL contracts.

The default image executes code from the 16 MiB Flash region at `0x20000000`.
Initialized data, BSS and the heap use the 16 KiB DSRAM bank at `0x01800000`,
while the stack uses the 16 KiB ISRAM bank at `0x01000000`; both banks are
data-accessible SRAM and together form a 32 KiB SRAM pool (they are not
contiguous, so they remain two linker regions). The 32 MiB SDRAM at
`0x80000000` is initialized by the startup code before `main` and registered
as the heap component's EXTERNAL pool, so large buffers can be allocated with
`ecos_heap_alloc(ECOS_HEAP_ZONE_EXTERNAL, ...)`; executing from SDRAM is not
part of the current Target profile.

Create and build a console example with:

```bash
python3 tools/ecos.py --sdk . project create hello --board t1-pico
cd hello
ecos build
```
