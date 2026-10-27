# USBFS controller driver

The CH32V103 backend initializes the USBFS controller, enables the documented
48 MHz PLL source (HSI/2 × 12), exposes controller status, and clears interrupt
flags. It does not provide USB descriptors, endpoint class logic, CDC/HID/MSC,
or a host stack. Those layers must be added separately.

The controller example is therefore an initialization/status smoke test, not a
USB enumeration pass. Enumeration and signal integrity require hardware.
