#ifndef SCR_H
#define SCR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define gotoyx(y, x) printf("\x1b[%d;%dH", (y), (x))

#define cursorfwd(x) printf("\x1b[%dC", (x))
#define cursorbwd(x) printf("\x1b[%dD", (x))

#define del_start_scr2c     printf("\x1b[1J")
#define del_c2eol           printf("\x1b[0K")
#define del_startline2c     printf("\x1b[1K")
#define del_ncharc2left(x)  printf("\x1b[%dP", (x))
#define del_ncharc2right(x) printf("\x1b[%dX", (x))

#define KEY_ESCAPE 0x001b
#define KEY_ENTER  0x000a
#define KEY_UP     0x0105
#define KEY_DOWN   0x0106
#define KEY_LEFT   0x0107
#define KEY_RIGHT  0x0108
#define DN         0x006a

#define bg_cyan  "\x1b[46m"
#define bg_reset "\x1b[49m"

static struct termios term, oterm;

static int getch(void);
static int kbhit(void);
static int kbesc(void);
int kbget(void);

int get_cursor_position(int ifd, int ofd, int *rows, int *cols);
int get_window_size(int ifd, int ofd, int *rows, int *cols);

#endif // SCR_H
