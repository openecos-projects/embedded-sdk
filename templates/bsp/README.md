# ECOS 3.0 BSP Template Boundary

New user BSPs are registered under the user-owned `boards/UserBSP` directory and
must provide an `ecos-board.yml` manifest. The SDK release does not copy user BSP
content or board-specific application sources.

Use the public Board schema and the Python CLI resource queries when creating a
BSP. The generated BSP must reference a Target by stable ID and expose logical
resources rather than embedding application code.
