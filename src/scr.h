#ifndef SCR_H
#define SCR_H

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
//#include <wchar.h>


#define save_state    "\033[?1049h\033[2J\033[H"
#define restore_state "\033[2J\033[H\033[?1049l"
#define del           "\033[%dX"
#define place_        "\033[%d;%dH"
#define del_line      "\033[2K"
#define n_els         "left_box.n_elements: %d"

#define move(fd, str)            write((fd), (str), strlen(str))
#define save_config              write((1), (save_state), sizeof(save_state))
#define restore_config           write((1), (restore_state), sizeof(restore_state))
#define del_from_cursor(str)     write((1), (str), strlen(str))
#define erase_scr(fd, str)       write((fd), (str), sizeof(str))


#define save_config_fd(fd)       write((fd), (save_state), sizeof(save_state))
#define restore_config_fd(fd)    write((fd), (restore_state), sizeof(restore_state))

#define test_text "TEST_TEXT"

#define PLACE_SZ sizeof(place_)
#define IN_SZ    sizeof(del)


char position[PLACE_SZ];
char del_in[IN_SZ];

#define write_line(fd, str) if (write((fd), (str), strlen(str)) < 0) { fprintf(stderr, "Error write\n"); }

/*
#define KEY_ESCAPE    0x001b
#define KEY_ENTER     0x000a
#define KEY_UP        0x0105
#define KEY_DOWN      0x0106
#define KEY_LEFT      0x0107
#define KEY_RIGHT     0x0108
#define KEY_SUPPR     0x0103
#define DN            0x006a
#define UP            0x006b
#define RIGHT         0x006c
#define LEFT          0x0068
#define ENTER         0x000d
#define KEY_BACKSPACE 0x007f

#define KEY_PAGE_UP 100
#define KEY_PAGE_DN 200
#define KEY_HOME    0x0300
#define KEY_END     0x0400
*/
                             // DECIMAL
#define KEY_ESCAPE    0x001b // 27
#define KEY_ENTER     0x000c // 13
#define KEY_UP        0x0103 // 259
#define KEY_DOWN      0x0102 // 258
#define KEY_LEFT      0x0104 // 260
#define KEY_RIGHT     0x0105 // 261
#define KEY_SUPPR     0x014a // 330
#define KEY_BACKSPACE 0x0008 // 8
#define KEY_PAGE_UP   0x0153 // 339
#define KEY_PAGE_DN   0x0152 // 338
#define KEY_HOME      0x0106 // 262
#define KEY_END       0x0168 // 360
#define KEY_ALL_UP    0x0061 // 97 equivalent to key_home

#define DN            0x006a
#define UP            0x006b
#define RIGHT         0x006c
#define LEFT          0x0068
//#define ENTER         0x000d
#define ENTER         0x000a
#define BACKSPACE     0x007f

#define bg_cyan         "\033[46m"
#define bg_blue         "\033[44m"
#define bg_light_blue   "\033[104m"
#define bg_reset        "\033[49m"

#define fg_bold         "\033[1m"
#define fg_bold_reset   "\033[21m"

#define fg_cyan         "\033[36m"
#define fg_blue         "\033[34m"
#define fg_yellow       "\033[93m"
#define fg_reset        "\033[39m"

#define r_triangle "\342\226\267"

#define r_full_triangle "\356\202\260"     // 
#define l_full_triangle "\356\202\262"     // 
#define r_line_triangle "\356\202\261"     // 
#define l_line_triangle "\356\202\263"     // 
#define r_white_arrow   "\360\237\242\226" // 🢖

#define folder_full_closed     "\356\227\277"     // 
#define folder_full_open       "\356\227\276"     // 
#define folder_clear_closed    "\360\237\227\200" // 🗀
#define folder_clear_open      "\360\237\227\201" // 🗁
#define folder_round_closed    "\357\201\273"     // 
#define folder_round_open      "\357\201\274"     // 
#define folder_full_j_closed   "\357\220\223"     // 
#define folder_round_closed_c  "\357\204\224"     // 
#define folder_round_open_c    "\357\204\225"     // 
#define bookmark               "\357\202\227"     // 
#define bookmark2              "\357\221\241"     // 
#define copy_files             "\357\203\205"     // 
#define move_folder            "\357\214\262"     // 
#define move_folder2           "\357\220\246"     // 


#define linux_penguin          "\356\234\222" // 
#define debian                 "\356\235\275" // 
#define ubuntu_clear           "\357\214\233" // 
#define ubuntu_full            "\357\214\234" // 
#define arch                   "\357\214\203" // 
#define centos                 "\357\214\204" // 
#define fedora_full            "\357\214\212" // 
#define fedora_clear           "\357\214\213" // 
#define gentoo                 "\357\214\215" // 
#define mint_clear             "\357\214\216" // 
#define mint_full              "\357\214\217" // 
#define mageia                 "\357\214\220" // 
#define manjaro                "\357\214\222" // 
#define nixos                  "\357\214\223" // 
#define open_suse              "\357\214\224" // 
#define raspberry_pi           "\357\214\225" // 
#define red_hat                "\357\214\226" // 
#define slackware_full         "\357\214\230" // 
#define slackware_clear        "\357\214\231" // 
#define gnome                  "\357\236\253" // 


#define git                    "\356\234\202" // 
#define git_cat                "\357\204\223" // 
#define git_full               "\357\207\222" // 
#define git_word               "\357\207\223" // 
#define merge                  "\357\204\246" // 
#define gitlab                 "\357\212\226" // 
#define slider_off             "\357\210\204" // 
#define slider_on              "\357\210\205" // 

#define music                  "\357\200\201"     // 
#define cle_sol                "\360\235\204\236" // 𝄞
#define video                  "\357\207\210"     // 
#define video2                 "\357\200\275"     // 
#define video3                 "\357\216\262"     // 
#define lock                   "\357\200\243"     // 
#define lock2                  "\357\221\226"     // 
#define lock_open              "\357\204\276"     // 
#define trash                  "\357\200\224"     // 
#define file_logo              "\357\200\226"     // 
#define file_logo2             "\357\211\212"     // 
#define file_logo3             "\357\211\211"     // 
#define file_logo4             "\357\242\231"     // 
#define file_logo5             "\357\205\233"     // 
#define mark_down              "\357\207\211"     // 
#define mark_down_alone        "\357\204\241"     // 
#define mark_down_full         "\357\234\255"     // 
#define gears                  "\357\202\205"     // 
#define save                   "\357\203\207"     // 
#define save2                  "\357\235\210"     // 
#define save3                  "\360\237\226\253" // 🖫
#define cissors                "\357\203\204"     // 
#define keyboard               "\357\204\234"     // 
#define keyboard2              "\342\214\250"     // ⌨
#define disks                  "\357\210\263"     // 
#define storage                "\357\216\225"     // 
#define search                 "\357\214\256"     // 
#define search2                "\357\220\242"     // 
#define adobe                  "\357\220\221"     // 
#define adobe2                 "\357\234\245"     // 
#define image1                 "\357\220\217"     // 
#define image2                 "\357\211\207"     // 
#define image3                 "\357\200\276"     // 
#define tar                    "\357\220\220"     // 
#define boomerang              "\360\220\207\241" // 𐇡
#define r_boomerang            "\356\212\205"     // 
#define word_document          "\357\207\202"     // 
#define word_document2         "\357\234\254"     // 
#define excel_document         "\357\207\203"     // 
#define power_point_document   "\357\207\204"     // 
#define power_point_document2  "\357\234\247"     // 
#define cancellation           "\360\237\227\231" // 🗙

#define settings               "\357\207\236" // 
#define settings2              "\357\214\261" // 
#define license_logo           "\357\220\275" // 
#define binary_file            "\357\221\261" // 

#define c_plus_plus            "\356\230\235" // 
#define c_file                 "\356\230\236" // 
#define terminal               "\356\236\225" // 
#define terminal2              "\357\232\214" // 
#define vim                    "\356\237\205" // 
#define vim2                   "\356\230\253" // 
#define prompt                 "\357\204\240" // 


#define two_bars_separation    "\356\271\276" // 
#define one_vertical_bar       "\356\272\220" // 
#define s_slash                "\356\274\204" // 
#define alert                  "\357\201\261" // 
#define alert_triangle         "\357\224\245" // 
#define alert_white            "\357\224\251" // 

#define lu_round_corner      "\342\225\255"
#define ru_round_corner      "\342\225\256"
#define ll_round_corner      "\342\225\260"
#define rl_round_corner      "\342\225\257" // "╯"

#define lu_corner            "\342\224\214" // "┌"
#define ll_corner            "\342\224\224" // "└"
#define ru_corner            "\342\224\220" // "┐"
#define rl_corner            "\342\224\230" // "┘"
#define line                 "\342\224\200" // "─"
#define v_line               "\342\224\202" // "│"

#define lu_heavy_corner      "\342\224\217"
#define ru_heavy_corner      "\342\224\223"
#define ll_heavy_corner      "\342\224\227"
#define rl_heavy_corner      "\342\224\233"
#define heavy_line           "\342\224\201"
#define heavy_v_line         "\342\224\203"
#define heavy_uppert_corner  "\342\224\263"
#define heavy_v_up           "\342\225\271"
#define heavy_lowert_corner  "\342\224\273"

#define box_color       1
#define box_thickness   1

#if box_color && box_thickness
    #define cont_0   6
    #define cont_1   7
    #define cont_2   8
    #define cont_3   9
    #define cont_4   10
    #define cont_5   11
    #define cont_6   12
    #define cont_7   13
    #define cont_8   14
#else
    #define cont_0   0
    #define cont_1   1
    #define cont_2   2
    #define cont_3   3
    #define cont_4   4
    #define cont_5   5
#endif // box_color && box_thickness

#define BOX_CONTOUR(...) const char *ARRAY[] = { __VA_ARGS__ }

typedef struct {
    int y_beg;
    int x_beg;
    int y_size;
    int x_size;
    int y_previous;
    int x_previous;
    unsigned short y_px_size;
    unsigned short x_px_size;
} Window_;

typedef struct {
    int array_size;
    int n_to_print;
    int n_lower_t;
    int pos_upper_t;
    int pos_lower_t;
    int option_previous;
} Scroll;

typedef struct {
  long keypress_value;
  unsigned long ascii_value;
  int n_times_pressed;
  int last_position_array;
  int last_element_is_not_img;
} InfoKeyPresses;

static char *TYPE[] = {
  "j", /* JPEG JIFF */
  "p", /* PNG       */
  "g", /* GIF       */
  "d", /* PDF       */
  "o"  /* FOLDER    */
};

typedef struct _STAT_INFO {
  unsigned short width;
  unsigned short height;
  //unsigned int file_len;
  unsigned long file_len;
  char *file_name;
  FILE *file;
  unsigned char *data;
} STAT_INFO;

static InfoKeyPresses info_key_presses;

#ifndef TRUE
#define TRUE  1
#endif // TRUE
#ifndef FALSE
#define FALSE 0
#endif // FALSE


static double elapsedTime;
static double pastElapsedTime;
static struct timeval t1, t2;
static volatile sig_atomic_t is_retriggered = 0;
static volatile sig_atomic_t n_times_keypressed = 0;
static volatile sig_atomic_t n_times_keypressed_copy = 0;

//typedef int bool;
static int quit = FALSE;
#define PRINT(msg) do {                                          \
  fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__); \
  fprintf(stdout, "%s\n", (msg));                                \
  exit(1);                                                       \
} while (0)
#define MALLOC(str, len) do {                                        \
  if (!((str) = malloc((len) * sizeof (*(str)))) && (quit = TRUE)) { \
    PRINT("malloc");                                                 \
  }                                                                  \
} while (0)
#define CALLOC(elements, capacity) do {                                          \
  if (!((elements) = calloc(capacity, sizeof (*(elements)))) && (quit = TRUE)) { \
    PRINT("calloc");                                                             \
  }                                                                              \
} while (0)
#define REALLOC(elements, capacity) do {                                                   \
  void *tmp = NULL;                                                                        \
  if (!(tmp = realloc((elements), (*(capacity)) * sizeof *(elements))) && (quit = TRUE)) { \
    PRINT("realloc");                                                                      \
  }                                                                                        \
  (elements) = tmp;                                                                        \
  tmp = NULL;                                                                              \
} while (0)
#define copy(dest, src, len) do {  \
  MALLOC((*(dest)), (len) + 1);    \
  memcpy((*(dest)), (src), (len)); \
  (*(dest))[(len)] = '\0';         \
} while (0)
#define combine(dest, src_one, src_two, len_one, len_two) do { \
  MALLOC((dest), (len_one) + (len_two) + 1);                   \
  memcpy((*dest), (src_one), (len_one));                       \
  memcpy(&((*dest)[len_one]), (src_two), len_two);             \
  (*dest)[(len_one) + (len_two)] = '\0';                       \
} while(0)


#ifndef __file_descriptor
#define __file_descriptor 1
#endif // __file_descriptor
#define mvprint_goback(vert, horiz, returnVert, returnHoriz, _str) do {                \
  mv((vert), (horiz));                                \
  write_line(__file_descriptor, _str);            \
  mv((returnVert), (returnHoriz));                               \
} while(0)
#define mvprint(vert, horiz, _str) do {                \
  mv((vert), (horiz));                                \
  write_line(__file_descriptor, _str);            \
  mv((vert), (horiz));                                \
} while(0)
#define mv(y, x) do {                          \
  sprintf(position, place_, (y), (x)); \
  move(__file_descriptor, position);             \
} while(0)
#define empty_space_debug(x_) do {                                        \
  int _k;                                                                \
  for (_k = 0; _k < x_; ++_k) { write_line(__file_descriptor, " "); } \
} while(0)
#define UNSIGNEDLONG "%lu"
static char unsignedlong[sizeof(UNSIGNEDLONG)];
#define printTTY_UL(_x, _y, _ul_number) do { \
  mv((_y), (_x));                 \
  sprintf(unsignedlong, UNSIGNEDLONG, _ul_number); \
  empty_space_debug(strlen((unsignedlong)));   \
  write_line(__file_descriptor, unsignedlong);            \
} while(0)
#define printTTYSTR(_x, _y, array) do { \
  mv((_y), (_x));                 \
  empty_space_debug(strlen((array)));   \
  mvprint((_y), (_y), (array));   \
} while(0)
#define NUMINT " = %d"
static char numint[sizeof(NUMINT)];
#define printTTYINT(_x, _y, _numint) do { \
  sprintf(numint, NUMINT, _numint);       \
  empty_space_debug(strlen((numint)));     \
  mvprint((_y), (_y), (numint));    \
} while(0)

struct termios term, oterm;

int getch(void);
int kbhit(void);
int kbesc(void);
int kbget(void);
void ttymode_reset(int mode, int imode);

int get_cursor_position(int ifd, int ofd, int *rows, int *cols);
int get_window_size(int ifd, int ofd, int *rows, int *cols);

#endif // SCR_H
