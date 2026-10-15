# PS2 keycodes

The L3.1 PS2 controller uses GPIO1[12] and GPIO1[13]. This example reads raw
scan codes from the controller and stops when Escape (`0x76`) is received.
