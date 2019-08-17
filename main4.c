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

//int sz = 14;
int sz = 42;
static char *entry[] = { "array.c", "array.h", "CMakeLists.txt", "coc-settings.json",
                         "main.c", "main2.c", "main3.c", "README.md", "scr.c", "scr.h",
                         "screen.c", "settings.json", "test", "test.c",
                         "array.c", "array.h", "CMakeLists.txt", "coc-settings.json",
                         "main.c", "main2.c", "main3.c", "README.md", "scr.c", "scr.h",
                         "screen.c", "settings.json", "test", "test.c",
                         "array.c", "array.h", "CMakeLists.txt", "coc-settings.json",
                         "main.c", "main2.c", "main3.c", "README.md", "scr.c", "scr.h",
                         "screen.c", "settings.json", "test", "test.c"  };

int main(void)
{
    setlocale(LC_ALL, "");
    printf("\x1b[?1049h\x1b[2J\x1b[H");

    int c = 0; int pos = 0; int len = 0; int maxlen = 60;

    Window w_1;
    int rows = 0;
    int cols = 0;
    if (get_window_size(STDIN_FILENO, STDOUT_FILENO, &rows, &cols)) {
        printf("\x1b[2J\x1b[H\x1b[?1049l");
        perror("Error getting window size\n");
        return EXIT_FAILURE;
    }

    w_1.y_size = rows;
    w_1.x_size = cols;
    w_1.y_beg = 5;
    w_1.x_beg = 5;

    Scroll s;
    s.array_size = sz;
    s.n_to_print = 10;
    s.n_upper_t = pos;
    s.n_lower_t = sz - s.n_to_print;

    s.pos_upper_t = pos;
    s.pos_lower_t = s.n_to_print - 1;

    for (int i = 0; i < s.n_to_print; ++i) {

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

    int previous_pos = pos;

    while (1) {
        if (debug_mode) {
            gotoyx(2, (w_1.x_size / 2) - 20);
            del_ncharc2right(42);
            printf("pos: %d, s.n_upper_t: %d, s.n_lower_t: %d",
                    pos, s.n_upper_t, s.n_lower_t);
            gotoyx(3, (w_1.x_size / 2) - 20);
            del_ncharc2right(48);
            printf("s.pos_upper_t: %d, s.pos_lower_t: %d, s.n_to_print: %d",
                    s.pos_upper_t, s.pos_lower_t, s.n_to_print);

            gotoyx(w_1.y_beg, (w_1.x_size / 2) - 20);
            printf("__________________ s.pos_upper_t: %d", s.pos_upper_t);

            gotoyx(w_1.y_beg + s.n_to_print - 1, (w_1.x_size / 2) - 20);
            printf("__________________ s.pos_lower_t: %d, s.n_lower_t: %d", 
                    s.pos_lower_t, s.n_lower_t);

            gotoyx(w_1.y_size - 1, w_1.x_size - 15);
            printf("y: %d, x: %d", w_1.y_size, w_1.x_size);
        }

        c = kbget();
        if (c == KEY_ESCAPE) {
            break;
        } else if (c == KEY_DOWN || c == DN) {
            if (pos < sz - 1) {
                int i = 0;
                ++pos;
                if (pos > s.n_to_print - 1 && s.pos_lower_t < sz) {

                    ++s.n_upper_t;
                    s.pos_upper_t = s.n_upper_t;
                    --s.n_lower_t;
                    s.pos_lower_t = pos;
                    gotoyx(1, 1);
                    printf("pos: %d > s.n_to_print: %d", pos, s.n_to_print);
                    for (i = 0; i < s.n_to_print; ++i) {
                        gotoyx(i + w_1.y_beg, w_1.x_beg);
                        //len = strlen(entry[s.n_upper_t + i - 1]);
                        del_ncharc2right(maxlen);
                        printf("%s", entry[s.n_upper_t + i]);
                    }
                    gotoyx(s.n_to_print + w_1.y_beg - 1, w_1.x_beg);
                    len = strlen(entry[pos]);
                    del_ncharc2right(maxlen);
                    for (i = 0; i < maxlen; ++i) {
                        if (i < len) {
                            printf("%s%c", bg_cyan, entry[pos][i]);
                        } else if (i == maxlen - 1) {
                            printf(" %s", bg_reset);
                        } else {
                            printf(" %s", bg_cyan);
                        }
                    }
                } else {

                    len = strlen(entry[pos - 1]);
                    gotoyx(pos + w_1.y_beg - 1, w_1.x_beg);
                    del_ncharc2right(len);
                    gotoyx(pos + w_1.y_beg - 1, w_1.x_beg);
                    printf("%s", entry[pos - 1]);

                    len = strlen(entry[pos]);
                    del_ncharc2right(maxlen);
                    gotoyx(pos + w_1.y_beg, w_1.x_beg);
                    
                    for (int i = 0; i < maxlen; ++i) {
                        if (i < len) {
                            printf("%s%c", bg_cyan, entry[pos][i]);
                        } else if (i == maxlen - 1) {
                            printf(" %s", bg_reset);
                        } else {
                            printf(" %s", bg_cyan);
                        }
                    }
                    
                }
            }
        }
    }
    printf("\x1b[2J\x1b[H\x1b[?1049l");
    return 0;
}


/*
 *else if (c == KEY_UP || c == UP) {
            if (pos > 0) {

                if (s.n_upper_t > 0) {
                    --s.n_upper_t;
                    ++s.n_lower_t;
                    s.pos_upper_t = pos;
                }


                previous_pos = pos;
                len = strlen(entry[pos]);
                gotoyx(pos + w_1.y_beg, w_1.x_beg);
                del_ncharc2right(len);
                printf("%s", entry[pos]);

                len = strlen(entry[pos - 1]);
                //--pos;
                del_ncharc2right(maxlen);

                gotoyx(pos-- + w_1.y_beg, w_1.x_beg);
                for (int i = 0; i < maxlen; ++i) {
                    if (i < len) {
                        printf("%s%c", bg_cyan, entry[pos][i]);
                    } else if (i == maxlen - 1) {
                        printf(" %s", bg_reset);
                    } else {
                        printf(" %s", bg_cyan);
                    }
                }

                //printf("%s%s%s", bg_cyan, entry[pos], bg_reset);
            }
        } 
 *
 */
