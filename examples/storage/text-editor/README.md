# Text editor

This development example mounts an existing FatFs volume and opens a Letter-Shell
prompt. On a terminal set to 80 columns and 24 rows, enter `edit /notes.txt` to
edit a file on drive `0:`. Arrow keys move the cursor; Enter, Backspace and Tab
edit text. Press `Ctrl+W` to save and `Esc` to exit. Unsaved changes require a
second `Esc` to discard.

The prompt also provides `ls [directory]`, `cd <directory>` and
`touch <filename>`. `touch` leaves an existing file unchanged. Relative `edit`
paths follow the directory shown in the prompt.

The example does not format Flash. Use `format_filesystem()` only after backing
up existing data. The editor is limited to 24 KiB per file and 8192 lines.
The component ships with SDK 3.0; real Flash I/O, terminal behavior and
persistence after power loss still require hardware validation.
