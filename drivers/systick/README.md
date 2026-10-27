# SysTick driver

The CH32V103 SysTick backend uses the 64-bit PFIC SysTick counter and IRQ12
with the ordinary ECOS software-context trap path. The WCH peripheral is
one-shot; the backend clears the counter and restarts it after each callback.
The example uses the documented HCLK/8 tick source and a 1-second compare.
