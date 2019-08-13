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

int get_cursor_position(int ifd, int ofd, int *rows, int *cols)
{
    char buffer[32];
    unsigned int i = 0;

    if (write(ofd, "\x1b[6n", 4) != 4) { return -1; }

    while (i < sizeof(buffer) - 1) {
        if (read(ifd, buffer + i, 1) != 1) { break; }
        if (buffer[i] == 'R') { break; }
        ++i;
    }
    buffer[i] = '\0';

    if (buffer[0] != KEY_ESCAPE || buffer[1] != '[') { return -1; }
    if (sscanf(buffer + 2, "%d;%d", rows, cols) != 2) { return -1; }
    return 0;
}

int get_window_size(int ifd, int ofd, int *rows, int *cols)
{
    struct winsize w_s;

    if (ioctl(1, TIOCGWINSZ, &w_s) == -1 || w_s.ws_col == 0) {
        int orig_row, orig_col, retval;

        retval = get_cursor_position(ifd, ofd, rows, cols);
        if (retval == -1) { return -1; }

        if (write(ofd, "\x1b[999C\x1b[999B", 12) != 12) { return -1; }
        retval = get_cursor_position(ifd, ofd, rows, cols);
        if (retval == -1) { return -1; }

        /* Restore position */
        char seq[32];
        snprintf(seq, 32, "\x1b[%d;%dH", orig_row, orig_col);
        if (write(ofd, seq, strlen(seq)) == -1) {
            /* Can't recover */
            return -1;
        }
        return 0;
    } else {
        *rows = w_s.ws_col;
        *cols = w_s.ws_row;
        return 0;
    }
    return -1;
}















