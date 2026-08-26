# AbstractMachine environment

This directory contains the SDK-facing AbstractMachine environment: project
rules, workload policy, and program staging. It is an optional runtime
environment built on the SDK HAL, not a standalone ysyx project.

Upstream sources are kept in `third_party/abstract-machine` and
`third_party/am-kernels`. Board-specific startup, linker, capability profiles,
and HAL bindings live under
`board/<board>/environments/abstract-machine`.
