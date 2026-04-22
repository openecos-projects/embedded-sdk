# ECOS Embedded SDK

English | [中文](README.md)

## Quick Start

### 1. Environment Setup
   Run the one-click install script: `./install.sh` 
   After the script finishes, reopen the terminal or run `source ~/.bashrc` to refresh environment variables.
### 2. Create an Example Project
In the directory where you want to create an example project, run: `ecos init_project <template_name>` 
Here `<template_name>` is the name of the example template you want to use, for example `hello`.

After the command completes, a subdirectory named after the template will be created in the current directory, containing all the files of the example project. 

The development board will be set to StarrySky C1 by default.

### 3. Create Configuration Files
In the example project directory, run: make menuconfig 
When using the C1 board, the configuration UI exits by default and immediately generates the configuration files required for compilation.

### 4. Build the Project
   In the example project directory, run: `make` 
   After compilation, the build directory will contain `retrosoc_fw.elf`, `retrosoc_fw.hex`, and `retrosoc_fw.bin`. Use the `.bin` file for flashing.


- Code Writing: [maksyuki](https://github.com/maksyuki/maksyuki)
- Maintenance & Code Writing: [XHTimmo](https://github.com/XHTimmo)
