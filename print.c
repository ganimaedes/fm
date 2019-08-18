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

int debug_mode = 1;

int sz = 29;
static char *entry[] = { "array.c", "array.h", "CMakeLists.txt", "coc-settings.json",
                         "main.c", "main2.c", "main3.c", "README.md", "scr.c", "scr.h",
                         "screen.c", "settings.json", "test", "test.c",
                         "boot", "grub", "memtest86+.bin", "memtest86+.elf",
                         "memtest86+_multiboot.bin", "grubenv", "unicode.pf2", "usr", "bin",
                         "alsaloop", "alsamixer", "alsatplg", "alsaucm", "amidi", "amixer" };

void highlight(char *entry[], int *maxlen, int *pos, int *len)
{
    int i;
    for (i = 0; i < *maxlen; ++i) {
        if (i < *len) {
            printf("%s%c", bg_cyan, entry[*pos][i]);
        } else if (i == *maxlen - 1) {
            printf(" %s", bg_reset);
        } else {
            printf(" %s", bg_cyan);
        }
    }
}

int main(void)
{
    setlocale(LC_ALL, "");
    printf("\x1b[?1049h\x1b[2J\x1b[H");

    int c = 0; int pos = 0; int len = 0; int maxlen = 60;

    Window w_1;

    if (get_window_size(STDIN_FILENO, STDOUT_FILENO, &w_1.y_size, &w_1.x_size)) {
        printf("\x1b[2J\x1b[H\x1b[?1049l");
        perror("Error getting window size\n");
        return EXIT_FAILURE;
    }

    w_1.y_beg = 5;
    w_1.x_beg = 5;

    Scroll s;
    s.array_size = sz;
    s.n_to_print = 10; // w_1.y_size;
    s.n_upper_t = pos;
    s.n_lower_t = sz - s.n_to_print;

    s.pos_upper_t = pos;
    s.pos_lower_t = s.n_to_print - 1;


    int SIZE = 0;
    if (s.n_to_print < sz) {
        SIZE = s.n_to_print;
    } else {
        SIZE = sz - 1;
    }

    int i = 0;
    int y = 0;

    for (i = 0; i < SIZE; ++i) {

        gotoyx(i + w_1.y_beg, w_1.x_beg);
        if (i == 0) {
            len = strlen(entry[i]);
            for (int j = 0; j < maxlen; ++j) {
                if (j < len) {
                    printf("%s%c", bg_cyan, entry[i][j]);
                } else if (j == maxlen - 1) {
                    printf(" %s", bg_reset);
                } else {
                    printf(" %s", bg_cyan);
                }
            }
        } else {
            printf("%s", entry[i]);
        }
    }



    while (1) {

        if (debug_mode) {
            gotoyx(2, (w_1.x_size / 2) - 20);
            del_ncharc2right(42);
            printf("pos: %d, s.n_upper_t: %d, s.n_lower_t: %d",
                    pos, s.n_upper_t, s.n_lower_t);
            gotoyx(3, (w_1.x_size / 2) - 20);
            del_ncharc2right(56);
            printf("s.pos_upper_t: %d, s.pos_lower_t: %d, s.n_to_print: %d",
                    s.pos_upper_t, s.pos_lower_t, s.n_to_print);

            gotoyx(w_1.y_beg, (w_1.x_size / 2) - 20);
            del_ncharc2right(44);
            printf("__________________ s.pos_upper_t: %d, y: %d", s.pos_upper_t, y);

            gotoyx(w_1.y_beg + s.n_to_print - 1, (w_1.x_size / 2) - 20);
            del_ncharc2right(54);
            printf("__________________ s.pos_lower_t: %d, s.n_lower_t: %d",
                    s.pos_lower_t, s.n_lower_t);

            gotoyx(w_1.y_size - 1, w_1.x_size - 15);
            printf("y: %d, x: %d", w_1.y_size, w_1.x_size);

            if (pos > s.n_to_print - 1 && s.pos_lower_t < sz) {
                gotoyx(1, 1);
                printf("pos: %d > s.n_to_print: %d", pos, s.n_to_print);
                gotoyx(1, 150);
                printf("%d", s.n_to_print + w_1.y_beg - 1);
            }
        }

        c = kbget();
        if (c == KEY_ESCAPE) {
            break;
        } else if (c == KEY_UP || c == UP) {
            if (pos > 0) {
                --pos;
                y = pos - s.pos_upper_t + w_1.y_beg;
                if (pos >= s.pos_upper_t && pos < s.pos_lower_t) { // NO SCROLL

                    gotoyx(y + 1, w_1.x_beg);
                    del_ncharc2right(maxlen);
                    printf("%s", entry[pos + 1]);
                    gotoyx(y, w_1.x_beg);

                } else if (pos <= s.pos_upper_t && s.pos_upper_t > 0) {
                    --s.pos_upper_t;
                    --s.pos_lower_t;
                    ++s.n_lower_t;
                    --s.n_upper_t;

                    // reprint all 10 decalees de 1 a partir de pos
                    int j = pos;
                    for (i = 0; i < SIZE; ++i, ++j) {
                        gotoyx(i + w_1.y_beg, w_1.x_beg);
                        del_ncharc2right(maxlen);
                        printf("%s", entry[j]);
                    }

                    gotoyx(pos - s.n_upper_t + w_1.y_beg, w_1.x_beg);
                    del_ncharc2right(maxlen);

                }
                len = strlen(entry[pos]);
                highlight(entry, &maxlen, &pos, &len);
            }
        } else if (c == KEY_DOWN || c == DN) {
            if (pos < sz - 1) {
                ++pos;
                y = pos - s.pos_upper_t + w_1.y_beg;
                if (pos > s.pos_upper_t && pos <= s.pos_lower_t) { // NO SCROLL


                    gotoyx(y - 1, w_1.x_beg);
                    del_ncharc2right(maxlen);
                    printf("%s", entry[pos  - 1]);
                    gotoyx(y, w_1.x_beg);

                } else if (pos - 1 >= s.pos_lower_t) {

                    ++s.n_upper_t;
                    s.pos_upper_t = s.n_upper_t;
                    --s.n_lower_t;
                    ++s.pos_lower_t;

                    for (i = 0; i < SIZE; ++i) {
                        gotoyx(i + w_1.y_beg, w_1.x_beg);
                        del_ncharc2right(maxlen);
                        printf("%s", entry[s.n_upper_t + i]);
                    }
                    gotoyx(s.n_to_print + w_1.y_beg - 1, w_1.x_beg);
                    del_ncharc2right(maxlen);

                }
                len = strlen(entry[pos]);
                highlight(entry, &maxlen, &pos, &len);
            }

        }
    }
    printf("\x1b[2J\x1b[H\x1b[?1049l");
    return 0;
}
