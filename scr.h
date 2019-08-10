#ifndef SCR_H
#define SCR_H

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define cursorfwd(x) printf("\x1b[%dC", (x))
#define cursorbwd(x) printf("\x1b[%dD", (x))

#define KEY_ESCAPE 0x001b
#define KEY_ENTER  0x000a
#define KEY_UP     0x0105
#define KEY_DOWN   0x0106
#define KEY_LEFT   0x0107
#define KEY_RIGHT  0x0108

static struct termios term, oterm;

static int getch(void);
static int kbhit(void);
static int kbesc(void);
static int kbget(void);

#endif // SCR_H
