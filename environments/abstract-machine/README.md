# AbstractMachine environment

This directory contains the SDK-wide build adapter for independent
AbstractMachine applications. It binds an application Makefile to the AM
runtime and to the AbstractMachine implementation declared by the selected
BSP.

The AM runtime lives in `third_party/abstract-machine`, reusable program
templates live in `templates/am-kernels`, and board-specific startup, linker,
core, and HAL bindings live in `board/<board>/environments/abstract-machine`.
Application templates contain no board-specific adaptation.
