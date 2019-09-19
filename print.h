#ifndef PRINT_H
#define PRINT_H

#include "array.h"
#include "scr.h"

typedef struct {
    int y_beg;
    int x_beg;
    int y_size;
    int x_size;
} Window;

typedef struct {
    int array_size;
    int n_to_print;
    int n_upper_t;
    int n_lower_t;
    int pos_upper_t;
    int pos_lower_t;
} Scroll;

static int debug_mode = 0;

/*
static int sz = 29;
static char *entry[] = { "array.c", "array.h", "CMakeLists.txt", "coc-settings.json",
                         "main.c", "main2.c", "main3.c", "README.md", "scr.c", "scr.h",
                         "screen.c", "settings.json", "test", "test.c",
                         "boot", "grub", "memtest86+.bin", "memtest86+.elf",
                         "memtest86+_multiboot.bin", "grubenv", "unicode.pf2", "usr", "bin",
                         "alsaloop", "alsamixer", "alsatplg", "alsaucm", "amidi", "amixer" };
*/

void highlight(char *entry[], int *maxlen, int *pos, int *len);
void print_no_scroll(Window *w, char *entry[], int *y, int *pos, int *maxlen, int up);
void print_scroll(Window *w, Scroll *s, char *entry[], int SIZE, int *pos, int *maxlen, int up);
void print_debug_info(Window *w, Scroll *s, int *pos, int *sz, int *y);
Scroll set_scroll(int pos, int sz);
void move_up(Window *w, Scroll *s, char *entry[], int *pos, int *y, int *maxlen, int SIZE, int len);
void move_dn(Window *w, Scroll *s, char *entry[], int *pos, int *y, int *maxlen, int SIZE, int len, int *sz);

#endif  // PRINT_H
