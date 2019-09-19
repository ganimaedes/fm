#ifndef SCR_H
#define SCR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
//#include <fcntl.h>
#include <locale.h>
#include <wchar.h>

#define save_state    "\033[?1049h\033[2J\033[H"
#define restore_state "\033[2J\033[H\033[?1049l"
#define del           "\033[%dX"
#define place         "\033[%d;%dH"

#define move(fd, str)            write((fd), (str), strlen(str))
#define save_config              write(STDOUT_FILENO, (save_state), sizeof(save_state))
#define restore_config           write(STDOUT_FILENO, (restore_state), sizeof(restore_state))
#define del_from_cursor(str)     write(STDOUT_FILENO, (str), strlen(str))
#define erase_scr(fd, str)       write((fd), (str), sizeof(str))

#define test_text "TEST_TEXT"

#define KEY_ESCAPE 0x001b
#define KEY_ENTER  0x000a
#define KEY_UP     0x0105
#define KEY_DOWN   0x0106
#define KEY_LEFT   0x0107
#define KEY_RIGHT  0x0108
#define DN         0x006a
#define UP         0x006b
#define RIGHT      0x006c
#define LEFT       0x0068
#define ENTER      0x000d

#define bg_cyan  "\x1b[46m"
#define bg_reset "\x1b[49m"

struct termios term, oterm;

int getch(void);
int kbhit(void);
int kbesc(void);
int kbget(void);

int get_cursor_position(int ifd, int ofd, int *rows, int *cols);
int get_window_size(int ifd, int ofd, int *rows, int *cols);

#endif // SCR_H
