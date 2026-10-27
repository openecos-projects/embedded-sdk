# Window watchdog driver

Configures the CH32V103 WWDG prescaler/window/counter and supports explicit
refresh/status operations. Like the independent watchdog, enabling WWDG is
not reversible until reset. The reset example intentionally stops feeding and
must only be run with a recovery/download path ready.
