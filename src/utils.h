#ifndef UTILS_H
#define UTILS_H
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <locale.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>
#include <limits.h>


#define STRINGIZE_NX(A) #A
#define STRINGIZE(A) STRINGIZE_NX(A)
#define INT_SIZE ((CHAR_BIT * sizeof(int)) / 3 + 2)
#define INTEGER "%d"
static char integer[INT_SIZE];
#define del_deb                 "\033[%dX"
#define IN_SZ_DEB    sizeof(del_deb)
#define place_in_deb            "\033[%d;%dH"
#define PLACE_SZ_DEB sizeof(place_in_deb)
static char del_in_deb[IN_SZ_DEB];
#define del_line_deb(fd)        if (write((fd), del_in_deb, sizeof(del_in_deb)) < 0) { exit(1); }
static char position_deb[PLACE_SZ_DEB + 27];

#define mv_debug_fd(_fd, y, x) do {              \
  sprintf(position_deb, place_in_deb, (y), (x)); \
  move(_fd, position_deb);                       \
} while(0)

#define write_line_debug(fd, str) if (write((fd), (str), strlen(str)) < 0) { fprintf(stderr, "Error write\n"); exit(1); }
#define print_debug_fd(_fd, _str) do { \
  write_line_debug(_fd, " = ");        \
  write_line_debug(_fd, _str);         \
} while(0)
#define TTYINTFD(_fd, _y, _x, _numint) do {                \
  sprintf(integer, INTEGER, _numint);                      \
  mv_debug_fd(_fd, (_y), (_x));                            \
  write_line_debug(_fd, STRINGIZE_NX(_numint));            \
  unsigned int len_numint = strlen(STRINGIZE_NX(_numint)); \
  empty_space_debug_fd(_fd, strlen((integer)) + 10);       \
  mv_debug_fd(_fd, (_y), (_x + len_numint + 3));           \
  print_debug_fd(_fd, (integer));                          \
  mv_debug_fd(_fd, (_y), (_x));                            \
} while(0)

#define empty_space_debug_fd(_fd, x_) do {              \
  int _k;                                               \
  for (_k = 0; _k < x_; ++_k) { write_line(_fd, " "); } \
} while(0)

#endif // UTILS_H
