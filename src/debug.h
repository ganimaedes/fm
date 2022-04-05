#ifndef DEBUG_H
#define DEBUG_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <fcntl.h>
#include <locale.h>
#include <sys/wait.h>
#include <errno.h>

int file_descriptor;
//static char *file_descriptor_str = NULL;

#define bg_cyan_         "\033[46m"
#define bg_reset_        "\033[49m"

#ifndef TRUE
  #define TRUE  1
#endif // TRUE
#ifndef FALSE
  #define FALSE 0
#endif // FALSE

#define save_state_debug          "\033[?1049h\033[2J\033[H"
#define restore_state_debug       "\033[2J\033[H\033[?1049l"
#define del_debug                 "\033[%dX"
#define place_in_debug            "\033[%d;%dH"
#define del_line_debug(fd)        if (write((fd), del_in_debug, sizeof(del_in_debug)) < 0) { fprintf(stderr, "Error write\n"); }
#define write_line_debug(fd, str) if (write((fd), (str), strlen(str)) < 0) { fprintf(stderr, "Error write\n"); }
#define save_config_fd(fd)        if (write((fd), (save_state_debug), sizeof(save_state_debug)) < 0) { fprintf(stderr, "Error write\n"); }
#define restore_config_fd(fd)     if (write((fd), (restore_state_debug), sizeof(restore_state_debug)) < 0) { fprintf(stderr, "Error write\n"); }
#define highlight_reset_debug(fd) if (write((fd), bg_reset_, sizeof(bg_reset_)) < 0) { fprintf(stderr, "Error write\n"); }
#define highlight_cyan_debug(fd)  if (write((fd), bg_cyan_, sizeof(bg_cyan_)) < 0) { fprintf(stderr, "Error write\n"); }

#define PLACE_SZ_DEBUG sizeof(place_in_debug)
#define IN_SZ_DEBUG    sizeof(del_debug)

static char position_debug[PLACE_SZ_DEBUG];
static char del_in_debug[IN_SZ_DEBUG];

#define _LINENUMBER ":%d"
static char _line_number[sizeof(_LINENUMBER)];
#define PRINTDEBUG(_msg) do {                      \
  write_line_debug(file_descriptor, __FILE__);     \
  write_line_debug(file_descriptor, __func__);     \
  sprintf(_line_number, _LINENUMBER, __LINE__);    \
  write_line_debug(file_descriptor, _line_number); \
  write_line_debug(file_descriptor, "\n");         \
  if (strlen(_msg) > 0) {                          \
    write_line_debug(file_descriptor, (_msg));     \
  }                                                \
  if (quit) { exit(1); }                           \
} while (0)

//static volatile sig_atomic_t x_pos_debug = 0;
//static volatile sig_atomic_t y_pos_debug = 0;

static int n_times_add_element_called = 0;

#define __PRINTDEBUG do {                          \
  mv_debug(y_pos_debug, x_pos_debug);              \
  write_line_debug(file_descriptor, __FILE__);     \
  write_line_debug(file_descriptor, ":");          \
  write_line_debug(file_descriptor, __func__);     \
  sprintf(_line_number, _LINENUMBER, __LINE__);    \
  write_line_debug(file_descriptor, _line_number); \
  write_line_debug(file_descriptor, "\n");         \
  mv_debug(y_pos_debug, x_pos_debug);              \
} while (0)

  //fprintf(file_descriptor, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__); \

#define mvprint_debug(vert, horiz, _str) do {                \
  mv_debug((vert), (horiz));                                \
  write_line_debug(file_descriptor, _str);            \
  mv_debug((vert), (horiz));                                \
} while(0)
#define mv_debug(y, x) do {                          \
  sprintf(position_debug, place_in_debug, (y), (x)); \
  move(file_descriptor, position_debug);             \
} while(0)
#define empty_space_debug(x_) do {                                        \
  int _k;                                                                \
  for (_k = 0; _k < x_; ++_k) { write_line_debug(file_descriptor, " "); } \
} while(0)
#define printTTYSTR(_x, _y, array) do { \
  mv_debug((_y), (_x));                 \
  empty_space_debug(strlen((array)));   \
  mvprint_debug((_y), (_y), (array));   \
} while(0)
  //int _pos = 2;                         \
  //if (_pos_array <= 1) { _pos = 1; }    \

#define NUMLU " = %lu"
static char numlu[sizeof(NUMLU)];
#define printTTYLONGUNSIGNED(_x, _y, _numlu) do { \
  int _pos = 2;                                   \
  sprintf(numlu, NUMLU, _numlu);                  \
  empty_space_debug(strlen((numlu)));             \
  mvprint_debug((_y), (_y), (numlu));             \
} while(0)
  //if (_pos_array <= 1) { _pos = 1; }              \

#define NUMINT " = %d"
static char numint[sizeof(NUMINT)];
#define printTTYINT(_x, _y, _numint) do { \
  int _pos = 2;                           \
  sprintf(numint, NUMINT, _numint);       \
  empty_space_debug(strlen((numint)));     \
  mvprint_debug((_y), (_y), (numint));    \
} while(0)

  //if (_pos_array <= 1) { _pos = 1; }      \

#endif // DEBUG_H
