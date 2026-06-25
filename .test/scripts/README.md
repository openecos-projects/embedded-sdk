# Test Scripts

This directory stores automated test entrypoints and helper scripts.

Planned files:

- `preflight.sh`
- `run-default-board-build.sh`
- `check-artifacts.sh`

Project creation rule:

- Test runners must create SDK projects only through user-callable `ecos` commands.
- Do not create projects by copying template files directly in test scripts.
