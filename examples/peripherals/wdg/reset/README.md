# Watchdog reset

This L3.1 example mirrors the legacy watchdog sequence: unlock the registers,
set prescaler and compare, arm control `0x5`, then pulse the feed register.
The board may reset while it is running.
