# Watchdog reset

This example exercises the independent watchdog and may reset the board.

On CH32V103, the IWDG uses the LSI clock and the hardware sequence is:

- select a legal divider (4, 8, 16, 32, 64, 128, or 256);
- write the 12-bit reload value;
- reload and enable the watchdog;
- optionally feed it while the application is alive.

CH32V103 IWDG cannot be disabled by software after enabling; a reset is required.
The current CH32 port maps `prescaler` to the hardware divider and `compare` to the
reload value. Do not run this example on production data without a reset/recovery plan.

Create and build the CH32 test:

```bash
ecos project create wdg-reset --board ch32v103c8t6
cd wdg-reset
ecos build
ecos flash
```

The current `main.c` intentionally arms the watchdog and then loops. On hardware,
expect an automatic reset after the configured timeout; capture the boot log and
reset reason with a separate diagnostic if needed. This is a destructive reset test,
not a normal smoke test.
