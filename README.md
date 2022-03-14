# fm

Fm is a terminal file manager/viewer for linux

To compile with gcc:

```bash
cd src/ && make && sudo make install
```

To execute:

```bash
fm "$HOME"
```

To leave press `<ESC>`

Keys

| Keys            | Associated Action |
| -------------   | :---------------: |
| `UP arrow or k` | select item above |
| `DN arrow or j` | select item below |
| `\<DEL>` or `dd`| delete file       |
| `c` or `yy`     | copy file         |
| `p`             | paste file        |
| `\<Enter>`      | enter into folder |
| `\<Backspace>`  | go up one folder  |
| `\<ESC>`        | quit program      |
