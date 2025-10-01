# ECOS Embedded SDK
## Quick Start
1. Config the riscv cross-compilation toolchain
    1. confirm the riscv toolchain is installed and added to the PATH environment variable
    2. match the riscv toolchain type in the Makefile ```utils/abstract-machine/scripts/riscv-mycpu.mk```

2. set the environment variable '''AM_HOME''' to the path of '''utils/abstract-machine'''
3. navigate to the src directory: `cd prog/src`
4. run the make script: `make`
    - use `make help` to get the compile format

## Supported Drivers
- I2C read/write
- Timer configuration and delay function
- SPI write

## Planned Drivers
- SPI read/write
- PWM
- GPIO
- ST7735 screen controller
