# CL1-2512 SoC Target

`cl1-2512` is the SDK 3.0 Target used by StartySky T1-Pico. It owns the
RV32IMC/ILP32 build configuration, reset entry, Flash-XIP linker layout,
register definitions, and the SoC implementations of the public HAL contracts.

The default image executes code from the 16 MiB Flash region at `0x20000000`.
Initialized data, BSS, and the stack use the 16 KiB DSRAM region at
`0x01800000`. The legacy T1-Pico SDRAM boot profile remains in the 2.x compatibility
BSP and is not selected by this Target.

Create and build a console example with:

```bash
python3 tools/ecos.py --sdk . project create hello --board t1-pico
cd hello
ecos build
```
