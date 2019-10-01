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
#define save_config              write((STDOUT_FILENO), (save_state), sizeof(save_state))
#define restore_config           write((STDOUT_FILENO), (restore_state), sizeof(restore_state))
#define del_from_cursor(str)     write((STDOUT_FILENO), (str), strlen(str))
#define erase_scr(fd, str)       write((fd), (str), sizeof(str))

#define test_text "TEST_TEXT"

#define PLACE_SZ sizeof(place)
#define IN_SZ    sizeof(del)

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

#define KEY_PAGE_UP 100
#define KEY_PAGE_DN 200
#define KEY_HOME    300
#define KEY_END     400

#define bg_cyan  "\033[46m"
#define bg_reset "\033[49m"
#define fg_cyan  "\033[36m"
#define fg_reset "\033[39m"

#define lu_round_corner "\342\225\255"
#define ru_round_corner "\342\225\256"
#define ll_round_corner "\342\225\260"
#define rl_round_corner "\342\225\257" // "╯"  

#define lu_corner       "\342\224\214" // "┌"
#define ll_corner       "\342\224\224" // "└"
#define ru_corner       "\342\224\220" // "┐"
#define rl_corner       "\342\224\230" // "┘"
#define line            "\342\224\200" // "─"
#define v_line          "\342\224\202" // "│"

#define lu_heavy_corner "\342\224\217"
#define ru_heavy_corner "\342\224\223"
#define ll_heavy_corner "\342\224\227"
#define rl_heavy_corner "\342\224\233"
#define heavy_line      "\342\224\201"
#define heavy_v_line    "\342\224\203"

#define box_color       1
#define box_thickness   1

#if box_color && box_thickness
    #define cont_0   6
    #define cont_1   7
    #define cont_2   8
    #define cont_3   9
    #define cont_4   10
    #define cont_5   11
#else
    #define cont_0   0
    #define cont_1   1
    #define cont_2   2
    #define cont_3   3
    #define cont_4   4
    #define cont_5   5
#endif // box_color && box_thickness

#define BOX_CONTOUR(...) const char *ARRAY[] = { __VA_ARGS__ }

//enum right_keys { KEY_PAGE_UP = 1, KEY_PAGE_DN = 2 };

struct termios term, oterm;

int getch(void);
int kbhit(void);
int kbesc(void);
int kbget(void);

int get_cursor_position(int ifd, int ofd, int *rows, int *cols);
int get_window_size(int ifd, int ofd, int *rows, int *cols);

#endif // SCR_H
