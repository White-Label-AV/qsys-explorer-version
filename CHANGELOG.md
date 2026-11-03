# Changelog

## 1.0.0 - 2026-11-04

First release.

- Explorer property handler for `.qsys`: shows the saving Designer version as
  **Comments** (`Q-SYS Designer 9.10.2`) and **Product version** (`9.10.2`).
  Both appear in the Details pane, tooltip and Details columns.
- `qsysver.exe` command-line tool, which prints the version of each file you pass it.
- `install.cmd` / `uninstall.cmd`, which elevate themselves, install to Program
  Files, upgrade in place and offer to restart Explorer.
- README covers adding the columns to one folder, a whole project tree, or every folder.
- Tested against designs saved by Designer 5.2 to 10.5.
