#include "scr.h"

static int getch(void)
{
    int c = 0;
    tcgetattr(STDIN_FILENO, &oterm);
    memcpy(&term, &oterm, sizeof(term));
    term.c_lflag &= ~(ICANON | ECHO);
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    c = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oterm);
    return c;
}

static int kbhit(void)
{
    int c = 0;
    
    tcgetattr(STDIN_FILENO, &oterm);
    memcpy(&term, &oterm, sizeof(term));
    term.c_lflag &= ~(ICANON | ECHO);
    term.c_cc[VMIN] = 0;
    term.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    c = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oterm);
    if (c != -1) { ungetc(c, stdin); }
    return c != -1 ? 1 : 0;
}

static int kbesc(void)
{
    int c = 0;
    
    if (!kbhit()) { return KEY_ESCAPE; }
    c = getch();
    if (c == '[') {
        switch ((c = getch())) {
            case 'A':
                c = KEY_UP;
                break;
            case 'B':
                c = KEY_DOWN;
                break;
            case 'C':
                c = KEY_LEFT;
                break;
            case 'D':
                c = KEY_RIGHT;
                break;
            default: 
                c = 0;
                break;
        }
    } else {
        c = 0;
    }
    if (c == 0) { while (kbhit()) { getch(); } }
    return c;
}

static int kbget(void)
{
    int c = getch();
    return c == KEY_ESCAPE ? kbesc() : c;
}

















