# Power driver

Provides CH32V103 backup-domain access, PVD configuration, wake-up flags, and
sleep/stop/standby entry. Low-power entry is intentionally not exercised by
compile tests because it suspends the CPU and depends on board wake-up wiring.
