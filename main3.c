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

    for (int i = 0; i < sz; ++i) {
        gotoyx(i + 1, 5);
        if (i == 0) {
            printf("%s%s%s", bg_cyan, entry[i], bg_reset);
        } else {
            printf("%s", entry[i]);
        }
    }

    int c = 0;
    int pos = 0;
    while (1) {
        c = kbget();
        if (c == KEY_ESCAPE) { 
            break; 
        } else if (c == KEY_DOWN || c == DN) {
            int len = strlen(entry[pos]);
            gotoyx(pos + 1, 5);
            del_ncharc2right(len);
            printf("%s", entry[pos]);
            gotoyx(++pos + 1, 5);
            printf("%s%s%s", bg_cyan, entry[pos], bg_reset);
        }
    }
    printf("\x1b[2J\x1b[H\x1b[?1049l");
    return 0;
} 
