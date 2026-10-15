# ECOS Embedded SDK for VS Code (preview)

This desktop and remote workspace extension is a client of the host-installed
`ecos` CLI. It requires CLI major version 3 and JSON contract version 1.0.
Install the SDK and its host CLI first; opening a project never downloads tools,
builds firmware, or changes project files.

The Explorer's **ECOS Projects** view shows the selected SDK, Board, Target,
profile, toolchain state, and build state for each project in a multi-root
workspace. Commands provide Board and profile selection, configure, build,
clean, flash, serial monitor, toolchain status and explicit installation.
`ECOS: Show Manifest Schema` is available while the workspace is untrusted.
Restricted Mode only runs project and resource queries; all commands that write
or access devices are disabled. The extension runs in the workspace extension
host, including Remote SSH, WSL, and Dev Containers.

The `ecos` Task Provider exposes configure, build, clean, flash, and monitor
tasks bound to specific project directories. Commands that return diagnostics
populate Problems. Builds use the CLI's CMake-generated
`build/compile_commands.json`; the extension does not maintain a second set of
compiler flags or modify VS Code settings.

Debugging is offered only when the selected Board or Target declares a `debug`
server address, the project has a verified ELF artifact, the pinned toolchain
provides GDB, and the Microsoft C/C++ debugger is installed. Start the declared
debug server before running **ECOS: Debug**. The extension does not provide a
debug adapter or start a server automatically.

On Windows, the SDK installer publishes `share/ecos/cli-launch.json` next to the
host CLI. The extension uses that descriptor to launch Python and the CLI script
directly, preserving argument boundaries for paths containing spaces or
non-ASCII characters. `ecos.cliPath` may point to a direct executable or the
installed `ecos.cmd` launcher.

Build and test locally:

```bash
npm ci
npm test
npm run package
```

The VSIX is written to `out/ecos-embedded-sdk-0.1.0.vsix`. It is a local preview
artifact; no Marketplace publisher or hardware validation is implied.
