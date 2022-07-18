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

To leave press `q`

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
| `q`              | quit program      |
| `<CTRL+u>`       | Scroll Up         |
| `<CTRL+d>`       | Scroll Down       |
