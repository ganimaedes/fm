# fm

Fm is a terminal file manager/viewer for linux with an integrated terminal image viewer.

To compile with gcc:

```bash
cd src/ && make && sudo make install
```

To execute:

```bash
fm "$HOME"
```

To leave press `<ESC>` or `q`

Keys

| Keys             | Associated Action |
| -------------    | :---------------: |
| `UP arrow` or `k`| select item above |
| `DN arrow` or `j`| select item below |
| `<Shift+D>`      | delete file/folder|
| `c` or `yy`      | copy file         |
| `p`              | paste file        |
| `<Enter>`        | enter into folder |
| `<Backspace>`    | go up one folder  |
| `<ESC>` or `q`   | quit program      |
| `<CTRL+U>`       | Scroll Up         |
| `<CTRL+D>`       | Scroll Down       |
