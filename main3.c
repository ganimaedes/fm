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


    int c = 0;
    while (1) {
        for (int i = 0; i < sz; ++i) {
            gotoyx(i + 1, 5);
            printf("%s", entry[i]);
        }
        c = kbget();
        if (c == KEY_ESCAPE) { break; }
    }
    printf("\x1b[2J\x1b[H\x1b[?1049l");
    return 0;
}
