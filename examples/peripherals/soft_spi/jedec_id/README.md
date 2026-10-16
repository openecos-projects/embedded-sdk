# Software SPI JEDEC ID

This example reads the three-byte JEDEC ID (`0x9f`) from an external SPI Flash
through the Board's `soft-spi-bus` resource. It uses GPIO bit-banging and only
the public `soft-spi`, BSP Console, and generated Board resource interfaces.

StarrySky C2 declares four GPIO pins for this example: CS GPIO0[0], SCK GPIO0[1],
MOSI GPIO0[2], and MISO GPIO0[3].
Connect an external 3.3 V SPI Flash with a common ground and the appropriate
pull-ups. Confirm the pins are accessible on the actual board revision before
running. These pins overlap with display control resources, so do not run the
display and this example on the same wiring. The pinout and peripheral response
still require hardware validation.

```bash
ecos project create soft-spi-jedec-id --board starrysky-c2
cd soft-spi-jedec-id
ecos build
```

The example reports `JEDEC ID: xx xx xx` once. `00 00 00` and `ff ff ff` often
indicate a missing device or wiring problem; compare the result with the Flash
datasheet. The Board resource selects SPI mode 0. Other modes can be exercised
with a suitable device by changing `mode` in the Board manifest.
