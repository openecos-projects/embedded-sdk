# ST7735 QSPI example

This example initializes the board's `display` resource through the public
ST7735 device API and cycles the panel through red, green, and blue fills.

On StarrySky L4C1 the display uses QSPI0 chip select 0, GPIO0[29] for DC,
GPIO0[30] for reset and GPIO0[31] for backlight. Both pins are configured as
outputs and held high during initialization. The QSPI data pins and clock are
configured by the L4 Target HAL.
