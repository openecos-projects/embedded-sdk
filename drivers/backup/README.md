# Backup register driver

Provides access to the CH32V103 backup-domain data registers. Values survive
normal resets while the backup domain remains powered, but a backup-domain
reset or power loss clears them. This behavior needs board-level validation.
