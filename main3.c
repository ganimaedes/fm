#include "array.h"
#include "scr.h"

typedef struct {
    int y_beg;
    int x_beg;
    int y_size;
    int x_size;
} Window;

int sz = 14;
static char *entry[] = { "array.c", "array.h", "CMakeLists.txt", "coc-settings.json",
                         "main.c", "main2.c", "main3.c", "README.md", "scr.c", "scr.h", 
                         "screen.c", "settings.json", "test", "test.c" };

int main(void)
{
    printf("\x1b[?1049h\x1b[2J\x1b[H");

    int c = 0; int pos = 0; int len = 0; int maxlen = 60;

    for (int i = 0; i < sz; ++i) {
        
        gotoyx(i + 1, 5);
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
        c = kbget();
        if (c == KEY_ESCAPE) { 
            break; 
        } else if (c == KEY_UP || c == UP) {
            if (pos > 0) {
                len = strlen(entry[pos]);
                gotoyx(pos + 1, 5);
                del_ncharc2right(len);
                printf("%s", entry[pos]);

                len = strlen(entry[pos - 1]);
                //--pos;
                del_ncharc2right(maxlen);
                
                gotoyx(pos--, 5);
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
        } else if (c == KEY_DOWN || c == DN) {
            if (pos < sz - 1) {
                len = strlen(entry[pos]);
                gotoyx(pos + 1, 5);
                del_ncharc2right(len);
                printf("%s", entry[pos]);

                len = strlen(entry[pos + 1]);
                del_ncharc2right(maxlen);

                gotoyx(++pos + 1, 5);
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
    }
    printf("\x1b[2J\x1b[H\x1b[?1049l");
    return 0;
} 
