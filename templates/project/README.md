# ECOS 3.0 Project Template Boundary

SDK 3.0 projects are created from an Example through the Python CLI:

```bash
ecos project create hello --board starrysky-l4-c1 --path ~/workspace
```

The generated project owns only application metadata and sources. Board, Target,
toolchain, Kconfig and linker inputs come from the selected SDK resources. This
directory is the stable home for future project-generation skeletons; it must not
contain one copy of an application per Board.

The historical files below `templates/` remain migration inputs for 2.x wrappers
and are not included in the SDK 3.0 release archive.
