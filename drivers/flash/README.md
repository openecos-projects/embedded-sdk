# On-chip Flash driver

`driver-flash` exposes CH32V103 64 KiB on-chip Flash with 1 KiB page erase
and half-word programming. Reads use the physical Flash alias and writes must
be aligned to a half-word and preceded by erase when changing 0 bits back to 1.

Flash erase/program operations are destructive and require real hardware
verification; the compile tests only check the link and API contract.
