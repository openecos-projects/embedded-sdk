# DMA driver

The CH32V103 backend exposes DMA1 channels 1..7 through `driver-dma`. Channel
numbers are zero-based in the ECOS API (`0` is DMA1 channel 1). DMA2 is not
claimed until the exact package/resource mapping is confirmed.

The driver configures transfer direction, byte/half-word/word widths,
increment modes, circular mode, priority and optional PFIC callbacks. A
successful build or register-model test does not prove a peripheral request
mapping or transfer on hardware.
