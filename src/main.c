#include "array.h"
#include "scr.h"
#include "props.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define _XOPEN_SOURCE

#include "parcours.h"
#include "deb.h"
#include "copy.h"
#include <execinfo.h>
#include <signal.h>
#include <stddef.h>
#include <semaphore.h>

#ifndef __STDC_ISO_10646__
#error "Wide chars are not defined as Unicode codepoints"
#endif

#define pg_up "pg_up"
#define pg_dn "pg_dn"

#define NX_CAT(A) #A
#define CAT(A) NX_CAT(A)

int debug = 0;
volatile sig_atomic_t file_pasted_signal = 0;
volatile sig_atomic_t file_moved_signal = 0;
volatile sig_atomic_t file_to_be_moved_signal = 0;
volatile sig_atomic_t position_before_copying_sig = 0;
volatile sig_atomic_t n_elements_to_erase = 0;

volatile sig_atomic_t outside_box = 0;
volatile sig_atomic_t resized = 0;
volatile sig_atomic_t reprint = 0;
volatile sig_atomic_t back_pressed = 0;
volatile sig_atomic_t enter_backspace = 1;
volatile sig_atomic_t enter_pressed = 0;
volatile sig_atomic_t image_used = 0;
volatile sig_atomic_t counter_for_image = 0;
volatile sig_atomic_t modify_pos_bc_image_used = 0;
volatile sig_atomic_t image_appeared = 0;
volatile sig_atomic_t previous_position_before_backspace = 0;
volatile sig_atomic_t delete_file_folder_request = 0;
int _file_descriptor_;
int _file_descriptor_2;
int fd_boxdbg;
int fd_attrdbg;
int debug_fd_box = 0;
int debug_fd_scroll_pos = 1;
int debug_c_pos = 1;
int y_position_boxdbg = 1;
int y_position_attrdbg = 1;
int n_passes_for_print_tty_2 = 0;
int initial_pass_for_w0 = 0;
int mode_normal = 1;
int mode_visual = 0;

char *deleted_file = NULL;

int y_pts_2 = 1;
static int fd = 0;
static int number_of_windows = 2;

#define del_debug                 "\033[%dX"
#define IN_SZ_DEBUG    sizeof(del_debug)
#define place_in_debug            "\033[%d;%dH"
#define PLACE_SZ_DEBUG sizeof(place_in_debug)
static char del_in_debug[IN_SZ_DEBUG];
#define del_line_debug(fd)        if (write((fd), del_in_debug, sizeof(del_in_debug)) < 0) { exit(1); }
static char position_debug[PLACE_SZ_DEBUG + 27];

#define print_debug_m(_str) do {              \
  write_line_debug(_file_descriptor_, " = "); \
  write_line_debug(_file_descriptor_, _str);  \
} while(0)
#define mv_debug(y, x) do {                          \
  sprintf(position_debug, place_in_debug, (y), (x)); \
  move(_file_descriptor_, position_debug);           \
} while(0)

#define NUMINTEGER "%d"
static char numinteger[ENOUGH_INT];

#define mverase(_y, _x, _win_size) do {     \
  mv((_y), (_x));                           \
  empty_space_debug_fd(1, (_win_size) - 2); \
  mv((_y), (_x));                           \
} while(0)

int y_pts = 1;
int x_pts = 1;
#define TTYINT(_y, _x, _numint) do {                          \
  sprintf(numinteger, NUMINTEGER, _numint);                   \
  mv_debug((_y), (_x));                                       \
  write_line_debug(_file_descriptor_, STRINGIZE_NX(_numint)); \
  unsigned int len_numint = strlen(STRINGIZE_NX(_numint));    \
  empty_space_debug(strlen((numinteger)) + 10);               \
  mv_debug((_y), (_x + len_numint + 3));                      \
  print_debug_m((numinteger));                                \
  mv_debug((_y), (_x));                                       \
} while(0)

#define TTYSTR(_x, _y, _word) do {    \
  mv_debug((_y), (_x));               \
  empty_space_debug(strlen(_word));   \
  mvprint_debug((_y), (_x), (_word)); \
} while(0)

#define write_word(_w, _len) if (write((_file_descriptor_), (_w), (_len)) < 0) { exit(1); }
#define TTYSTR2(_y, _x, _word) do {                \
  mv_debug((_y), (_x));                            \
  write_line_debug(_file_descriptor_, CAT(_word)); \
  write_line_debug(_file_descriptor_, " = ");      \
  unsigned int len_word = strlen(CAT(_word));      \
  write_word(del_line, sizeof(del_line));          \
  mv_debug((_y), (_x + len_word));                 \
  print_debug_m((_word));                          \
} while(0)

#define mv_debug_fd(_fd, y, x) do {                  \
  sprintf(position_debug, place_in_debug, (y), (x)); \
  move(_fd, position_debug);                         \
} while(0)

#define print_debug_fd(_fd, _str) do { \
  write_line_debug(_fd, " = ");        \
  write_line_debug(_fd, _str);         \
} while(0)

#define TTYSTRFD(_fd, _y, _x, _word) do { \
  mv_debug_fd(_fd, (_y), (_x));           \
  print_debug_fd(_fd, (_word));           \
} while(0)

#define empty_space_debug_fd(_fd, x_) do {              \
  int _k;                                               \
  for (_k = 0; _k < x_; ++_k) { write_line(_fd, " "); } \
} while(0)

#define TTYINTFD(_fd, _y, _x, _numint) do {                \
  sprintf(numinteger, NUMINTEGER, _numint);                \
  mv_debug_fd(_fd, (_y), (_x));                            \
  write_line_debug(_fd, STRINGIZE_NX(_numint));            \
  unsigned int len_numint = strlen(STRINGIZE_NX(_numint)); \
  empty_space_debug_fd(_fd, strlen((numinteger)) + 10);    \
  mv_debug_fd(_fd, (_y), (_x + len_numint + 3));           \
  print_debug_fd(_fd, (numinteger));                       \
  mv_debug_fd(_fd, (_y), (_x));                            \
} while(0)

#define TTYINTFD2(_fd, _y, _x, _numint) do {               \
  sprintf(numinteger, NUMINTEGER, _numint);                \
  mv_debug_fd(_fd, (_y), (_x));                            \
  write_line_debug(_fd, STRINGIZE_NX(_numint));            \
  unsigned int len_numint = strlen(STRINGIZE_NX(_numint)); \
  empty_space_debug_fd(_fd, strlen((numinteger)) + 10);    \
  mv_debug_fd(_fd, (_y), (_x + len_numint + 3));           \
  print_debug_fd(_fd, (numinteger));                       \
  mv_debug_fd(_fd, (_y) + 1, (_x));                        \
} while(0)

#define TTYSTRLEN(_y, _x, _word, _len) do {        \
  mv_debug((_y), (_x));                            \
  write_line_debug(_file_descriptor_, CAT(_word)); \
  write_line_debug(_file_descriptor_, " = ");      \
  unsigned int len_word = strlen(CAT(_word));      \
  write_word(del_line, sizeof(del_line));          \
  mv_debug((_y), (_x + len_word));                 \
  write_word(_word, _len);                         \
} while(0)

#define NLINETAB if (write(_file_descriptor_2, "\n\t", strlen("\n\t")) < 0) { exit(1); }
#define _comma_ write_line_debug(_file_descriptor_2, ":")
#define _LINENUMBER_ "%d"
static char _line_number_[ENOUGH_INT];
#define _PRINTDEBUG do {                                        \
  write_line_debug(_file_descriptor_2, __FILE__); _comma_;      \
  write_line_debug(_file_descriptor_2, __func__); _comma_;      \
  sprintf(_line_number_, _LINENUMBER_, __LINE__);               \
  write_line_debug(_file_descriptor_2, _line_number_); NLINETAB \
} while (0)

#define mvfd(_fd, y, x) do {           \
  sprintf(position, place_, (y), (x)); \
  move(_fd, position);                 \
} while(0)


Window_ w_main, w1, w2, w3, w0;
//Attributes attributes;
Attributes *attributes;
Attributes *w0_attributes;

static int previous_pos_copy = 0;
static int previous_pos_copy_from_attr = 0;

typedef struct _Message {
  char *print_msg;
  char *n_char;
  int used_char;
  int n_int;
  int used_int;
  unsigned int n_uint;
  int used_uint;
  unsigned long n_ulong;
  int used_ulong;
} Message;

sem_t mutex;
volatile sig_atomic_t result_print_image = 0;

void print_path(Scroll *s, char *path, int pos, int backspace_pressed);
int strpos(char *hay, char *needle, int offset);
int show_tar(pid_t *pid, char *buffer, int *bytes_read, char *tar_name);
int match_extension(char *name, const char *ext);
void handler(int sig);
void reprint_menu(Window_ *w, Scroll *s, Array *a, Attributes *attr, int pos, int option);
void copy_scroll(Scroll *s_in, Scroll *s_out);
void indicators(Window_ *w, int y, int x, char pos_c[], char in[], char *msg);
void erase_window(Window_ *w, Scroll *s);
int print_logos(char *name, char *type);
int update(Window_ *w, Scroll *s, int *pos, int size);
void print_debug(Window_ *w, Scroll *s, int option, int pos, int cursor_pos, Array *a);
void move_erase(Window_ *w, int fd, int y, int x);
void print_entries(Window_ *w, Scroll *s, char **entries, int option, int c, int *pos, Array *a);
void print(Window_ *w, Array *a, int pos_array);
static void sig_win_ch_handler(int sig);
void mvwprintw(Window_ *win, Array *a, int y, int x, char *str, int pos);
void draw_box(Window_ *w);
void print_attributes_debug(Window_ *w, Scroll *s, int option, int pos,
    int cursor_pos, Array *a, Attributes *attributes, int fd);
int copy_file3(Array **left_box, int pos);
int read_tar(Array *left_box, int *pos);
int getBackSpaceFolder6(Array **left_box, Window_ *w, int *pos, int *previous_pos, Scroll *s);
int directory_placement2(Array *left_box, Array **right_box, Scroll *s, int *pos, Window_ *w1, Window_ *w2, Window_ *w_main);
int mv_to_trash3(Window_ *w1, Scroll *s, Array **left_box, int *pos, int *option);
void find_user(char *in, char **user);
void print_n_elements(Array *left_box);
void print_permissions(Array *a, Scroll *s1, Window_ *w, int pos);
void print_message2(Window_ *w, Scroll *s, int position_from_end_scr, int pos, Message *msg);
void read_file2(Array *left_box, Window_ *w1, Window_ *w2, Scroll *s, int pos);
int print_right_window3(Array **left_box,
                         Array **right_box,
                         Scroll *s,
                         Window_ *w1,
                         Window_ *w2,
                         Window_ *w_main,
                         Message *msg,
                         STAT_INFO *info_file,
                         int pos, int *c);
void open_file(STAT_INFO *info);
char find_file_type(STAT_INFO *info, char *file_name);
int strpos4(char *hay, char *needle, int offset);
int show_image(pid_t *pid, char *buffer, int *bytes_read, char *img_path);
int window_resize2(Window_ *w_main, Window_ *w0, Window_ *w1, Window_ *w2,
                  Array *left_box, int n_windows, int *previous_val_n_windows,
                  int first_window_width_fraction,
                  struct winsize *w_s,
                  Scroll *s,
                  int *pos, int *initial_loop, int *option, int *i);
int horizontal_navigation(int *c, int *pos, int *n_windows,
//int horizontal_navigation(long unsigned *c, int *pos, int *n_windows,
                          int *first_window_width_fraction,
                          Array **left_box, Array **right_box, Array **w0_left_box, Attributes **attributes, Attributes **w0_attributes,
                          Window_ *w0, Window_ *w1, Window_ *w2, Positions *posit,
                          int position_before_copying, int *previous_pos, Scroll *s, Scroll *w0_s,
                          int *second_previous_c, int *previous_pos_c, int *option, int *secondary_loop, int *previous_pos_w0, int *pos_w0,
                          int *left_allocation, int *backspace);
void initialize_sigwinch(struct sigaction *sact);
void initialize_windows(Window_ *w_main, Window_ *w0, Window_ *w1, Window_ *w2);
void highlight4(Window_ *w, Array *a, int *pos);
void find_parent(char *parent, char **real_parent);
void reprint_menu_3windows2(Array **a, Array *selected, Window_ *w, Attributes *attr, int pos, int option, int previous_pos, Scroll *s_w0);
void print_attributes(Attributes *attr, int *y_pts_2);
void print_all_attributes(Attributes *attr, int *y_pts_2);
void print_all_attributes_fd(int fd, Attributes *attr, int *y_position);
void print_box_fd(int fd, Array *box, int *y_position);
void scroll_window3(Window_ *w, Array *box, Scroll *s, int *pos);
void scroll_window4(Window_ *w, Array *box, Scroll *s, int *pos);
void scroll_window5(Window_ *w, Array *box, Scroll *s, int *pos);
void scroll_window_up7(Window_ *w, Array *box, Scroll *s, int *pos);
void print_scroll(Scroll *s, int *pos, int *y_position);
void ask_user(char *warning, int *c);
int show_all_85();
int show_all_855(int y, int x);
int show_all_8555(int y, int x);
void show_status_line(Window_ *w, Array *a, Scroll *s, int pos);

int main(int argc, char **argv)
{
  signal(SIGSEGV, handler);
  if (argc < 2) { fprintf(stderr, "Missing Arguments\n"); exit(1); }
  setlocale(LC_ALL, "");
  save_config;
  sem_init(&mutex, 0, 1);

  char position[strlen(place_)];
  if (get_window_size(0, 1, &w_main.y_size, &w_main.x_size) < 0) {
    restore_config;
    char *error = "Error getting window size";
    sprintf(position, place_, w1.y_beg + 1, w1.x_beg + 1);
    move(1, position);
    write(1, error, strlen(error));
    return EXIT_FAILURE;
  }

#if defined(PRINT_OTHERTTY)
  if (argc > 2 && argv[2] != NULL) {
    _file_descriptor_ = open(argv[2], O_RDWR);
    save_config_fd(_file_descriptor_);
  }
#endif // PRINT_OTHERTTY

#if defined(PRINT_OTHERTTY_2)
  if (argc > 3 && argv[3] != NULL) {
    _file_descriptor_2 = open(argv[3], O_RDWR);
    save_config_fd(_file_descriptor_2);
  }
#endif // PRINT_OTHERTTY_2

#if defined(BOXDBG)
  if (argc > 2 && argv[2] != NULL) {
    if (argv[3] != NULL) {
      fd_attrdbg = open(argv[3], O_RDWR);
      save_config_fd(fd_attrdbg);
    }
    if (debug_fd_box) {
      fd_attrdbg = open(argv[2], O_RDWR);
      save_config_fd(fd_attrdbg);
    } else {
      fd_boxdbg = open(argv[2], O_RDWR);
      save_config_fd(fd_boxdbg);
    }
  }
#endif // BOXDBG

  int pos = 0;

  initialize_windows(&w_main, &w0, &w1, &w2);

  Scroll s;
  s.option_previous = 0;

  Scroll s_w0;
  s_w0.option_previous = 0;

  struct winsize w_s;
  struct sigaction sact;
  initialize_sigwinch(&sact);

  int c;
  //long unsigned c;
  int previous_pos_c = 0;
  int second_previous_c = 0;
  int option = 0,
      i,
      initial_loop = 1,
      secondary_loop = 0,
      previous_pos = pos,
      left_allocation = 1;
  int backspace = 0;

  Array *left_box = NULL;
  Array *right_box = NULL;
  Array *w0_left_box = NULL;
  initialize_array(&left_box, 5);
  initialize_array(&right_box, 5);
  initialize_array(&w0_left_box, 5);

  Positions posit;
  posit.m_lower_pos = 0;
  posit.m_position = 0;
  posit.m_upper_pos = 0;

  attributes = NULL;
  initialize_attr(&attributes, 2);
  w0_attributes = NULL;
  initialize_attr(&w0_attributes, 2);

  parcours(argv[1], 0, left_box, 0, &w_main);

  int position_before_copying = 0;

  int yank_counter = 0;

#if defined(EBUG)
  if (argv[2] != NULL) {
    fd = open(argv[2], O_RDWR);
    save_config_fd(fd);
  }
#endif // EBUG

  Message msg = {};
  msg.used_int = 0;
  msg.used_uint = 0;
  msg.used_ulong = 0;
  msg.used_char = 0;

  info_key_presses.keypress_value = 0;
  info_key_presses.n_times_pressed = 0;

  STAT_INFO info_file = { 0 };

  int n_windows = 2;
  int previous_value_n_windows = n_windows;
  int first_window_width_fraction = 2; // for 1/3

  int pos_for_w0 = pos;

  int previous_pos_w0 = pos;

  mode_normal = 1;
  mode_visual = 0;

  for (;;) {
    if (!ioctl(0, TIOCGWINSZ, &w_s)) {
      sem_wait(&mutex);
      window_resize2(&w_main, &w0, &w1, &w2, left_box, n_windows, &previous_value_n_windows,
                     first_window_width_fraction, &w_s, &s, &pos, &initial_loop, &option, &i);
      sem_post(&mutex);
    }


#if defined(EBUG)
    msg.print_msg = "previous_pos = ";
    msg.n_ulong = previous_pos;
    msg.used_ulong = 1;
    print_message2(&w_main, &s, 1, pos, &msg);
#endif // EBUG

#if defined(BOXDBG)
    if (debug_fd_box) {
      //print_all_attributes_fd(fd_attrdbg, attributes, &y_position_attrdbg);
      print_all_attributes_fd(fd_attrdbg, w0_attributes, &y_position_attrdbg);
      //print_box_fd(fd_boxdbg, left_box, &y_position_boxdbg);
    }
#endif // BOXDBG

    if (left_box->n_elements != 0) {
      if (previous_pos < left_box->n_elements) {
        if (backspace) {
          pos = previous_pos;
          if (number_of_windows == 3) {
            pos_for_w0  = previous_pos_w0;
          }
          backspace = 0;
        }
      }

      if (!left_allocation || secondary_loop) {
        reprint = 1;
        secondary_loop = 0;
      }

      if (c == KEY_ENTER || c == KEY_BACKSPACE || resized || c == ENTER || c == BACKSPACE) {

        previous_pos_copy = previous_pos;
        if (number_of_windows == 3) {
          previous_pos_w0 = pos_for_w0;
        }
        if (attributes->n_elements > 0) {
          previous_pos_copy_from_attr = attributes->pos[attributes->n_elements - 1]->m_position;

          if (file_pasted_signal) {
            position_before_copying = pos;
            position_before_copying_sig = 1;
            pos = previous_pos_copy_from_attr;
            file_pasted_signal = 0;
          }
        }
        if (c == BACKSPACE && image_cp_signal == 1) {
          image_cp_signal = 0;
        }
        if (c != 0) {
          reprint_menu(&w1, &s, left_box, attributes, pos, option);
        }
        if (number_of_windows == 3) {
#if defined(PRINT_OTHERTTY)
          y_pts = 1;
          TTYINT(y_pts, 1, pos_for_w0); ++y_pts;
#endif // PRINT_OTHERTTY

          if (c != 0) {
            reprint_menu(&w0, &s_w0, w0_left_box, w0_attributes, pos_for_w0, option);
          }
        }
      }
      if (c == 'c' || c == 'x' || c == 'y' && image_cp_signal == 0) {
        if (c == 'y') {
          ++yank_counter;
        }
        if (yank_counter != 2) {
          if (file_to_be_copied && image_cp_signal == 0) {
            free(file_to_be_copied);
            file_to_be_copied = NULL;
          }
          if (image_cp_signal == 0) {
            size_t len_copy = strlen(left_box->menu[pos].complete_path);
            copy(&file_to_be_copied, left_box->menu[pos].complete_path, len_copy);
            left_box->menu[pos].is_marked = 1;
            if (c == 'x') {
              file_to_be_moved_signal = 1;
            }
            yank_counter = 0;
          }
        }

        image_cp_signal = 1;
        image_cp_pos = pos;
        mv(pos - s.pos_upper_t + w1.y_beg + 1, w1.x_beg + 1);
        highlight4(&w1, left_box, &pos);
      } else if (c == 'p') {
        int result_copy = copy_file3(&left_box, pos);
        if (file_to_be_moved_signal) {
          unlink(file_to_be_copied);
          file_to_be_moved_signal = 0;
        }
        if (result_copy && file_to_be_copied) {
          free(file_to_be_copied);
          file_to_be_copied = NULL;
        }

        n_elements_to_erase = left_box->n_elements;

        erase_window(&w1, &s);
        reprint_menu(&w1, &s, left_box, attributes, pos, option);
        left_box->menu[pos].is_marked = 0;
        file_pasted_signal = 1;
        if (image_cp_signal) {
          image_cp_signal = 0;
        }
      } else if (c == 'D') {
        mv(w_main.y_size - 1, w_main.x_beg + 1);
        if (file_to_be_copied != NULL) {
          free(file_to_be_copied);
          file_to_be_copied = NULL;
        }
        size_t len_copy = strlen(left_box->menu[pos].complete_path);
        copy(&file_to_be_copied, left_box->menu[pos].complete_path, len_copy);
        if (*(left_box->menu[pos].type) == 'd') {
          char *warning = "Delete this directory and all of its contents? (Y/n): ";
          ask_user(warning, &c);
          if (delete_file_folder_request == 1) {
            mv_to_trash3(&w1, &s, &left_box, &pos, &option);
            //int result_copy = copy_file3(&left_box, pos);
            delete_file_folder_request = 0;
          }
          mv(w_main.y_size - 1, w_main.x_beg + 1);
          del_from_cursor(del_in);
        } else if (*(left_box->menu[pos].type) == 'f') {
          char *warning = "Delete this file? (Y/n): ";
          ask_user(warning, &c);
          if (delete_file_folder_request == 1) {
            mv_to_trash3(&w1, &s, &left_box, &pos, &option);
            delete_file_folder_request = 0;
          }
          mv(w_main.y_size - 1, w_main.x_beg + 1);
          del_from_cursor(del_in);
        }
      }
      if (c != 0 && c != KEY_ESCAPE && c > 0) {
        print_entries(&w1, &s, NULL, option, (int)c, &pos, left_box);
      }

#if defined(BOXDBG)
    if (debug_fd_scroll_pos) {
      int y_position = 10;
      if (initial_loop == 0) {
        print_scroll(&s, &pos, &y_position);
      }
    }
#endif // BOXDBG
      //image_used = 0;
      //print_permissions(left_box, &s, &w1, pos);
#if defined(EBUG)
      msg.n_ulong = c;
      msg.used_ulong = 1;
      msg.print_msg = "keypress = ";
      print_message2(&w1, &s, 0, pos, &msg);
#endif // EBUG

      if (resized == 0 && image_used == 0 && c != 0 && c != KEY_ESCAPE && c > 0) {
        result_print_image = print_right_window3(&left_box, &right_box, &s, &w1, &w2, &w_main, &msg, &info_file, pos, &c);
      }
      //if (c == KEY_ESCAPE) { break; }
      if (c == KEY_Q) {
        break;
      } else if (c == KEY_ESCAPE) {
        //c = 'r';
        //c = 1;
        //break;
      //} else if (c == 0) {
        //sleep(5);
        //resized = 0;
        //continue;
      }
    }
    if (number_of_windows == 2) {
      show_status_line(&w1, left_box, &s, pos);
    } else if (number_of_windows == 3) {
      show_status_line(&w0, left_box, &s, pos);
    }
    //print_permissions(left_box, &s, &w1, pos);

    if (enter_backspace == 1 && attributes->n_elements != 0 && back_pressed == 1 && initial_loop != 1) {
      if (number_of_windows == 3 && w0_attributes->n_elements > 0) {
        free_attr(w0_attributes);
        initialize_attr(&w0_attributes, 1);
      }
      free_attr(attributes);
      initialize_attr(&attributes, 1);
    }

#if defined(EBUG)
      int p = pos - s.pos_upper_t + w1.y_beg + 1;
      print_debug(&w_main, &s, option, pos, p, &left_box);
      if (argv[2] != NULL && initial_loop == 1) {
        fd = open(argv[2], O_RDWR);
        save_config_fd(fd);
      }
      print_tty(&w3, fd, &attributes);
#endif // EBUG

    if (resized == 0) {
      int loop = 0;
      loop = horizontal_navigation((int *)&c, &pos, &n_windows, &first_window_width_fraction, &left_box, &right_box, &w0_left_box, &attributes,
      //loop = horizontal_navigation((long unsigned *)&c, &pos, &n_windows, &first_window_width_fraction, &left_box, &right_box, &w0_left_box, &attributes,
                            &w0_attributes, &w0, &w1, &w2, &posit,
                            position_before_copying, &previous_pos, &s, &s_w0, &previous_pos_w0, &pos_for_w0,
                            &second_previous_c, &previous_pos_c, &option, &secondary_loop, &left_allocation, &backspace);
      if (loop == 0) {
        break;
      }
      //TTYINTFD(1, 30, 1, c);
    }

    if (resized == 1 || enter_backspace && c != 0 && c != KEY_ESCAPE && c > 0) {
      erase_window(&w2, &s);
    }
    initial_loop = 0;

    if (resized == 1) {
        resized = 0;
    }
    if (image_used) {
      image_used = 0;
    }
    y_pts = 1;
  }
  if (left_box->n_elements != 0 || left_box->capacity != 0) {
    free_array(left_box);
  }
  if (right_box->n_elements != 0 || right_box->capacity != 0) {
    free_array(right_box);
  }
  if (w0_left_box->n_elements != 0 || w0_left_box->capacity != 0) {
    free_array(w0_left_box);
  }
  if (attributes->n_elements != 0 || attributes->capacity != 0) {
    free_attr(attributes);
  }
  if (w0_attributes->n_elements != 0 || w0_attributes->capacity != 0) {
    free_attr(w0_attributes);
  }
  if (file_to_be_copied) {
    free(file_to_be_copied);
    file_to_be_copied = NULL;
  }
#if defined(EBUG)
  if (argv[2] != NULL) {
    restore_config_fd(fd);
  }
#endif // EBUG
#if defined(PRINT_OTHERTTY)
  if (argc > 2 && argv[2] != NULL) {
    restore_config_fd(_file_descriptor_);
  }
#endif // PRINT_OTHERTTY
#if defined(PRINT_OTHERTTY_2)
  if (argc > 3 && argv[3] != NULL) {
    restore_config_fd(_file_descriptor_2);
  }
#endif // PRINT_OTHERTTY_2
#if defined(BOXDBG)
  if (argc > 2 && argv[2] != NULL) {
    if (argc == 3) {
      restore_config_fd(fd_attrdbg);
    }
    if (debug_fd_box) {
      restore_config_fd(fd_attrdbg);
    } else {
      restore_config_fd(fd_boxdbg);
    }
  }
#endif // BOXDBG
  restore_config;
  return 0;
}

void print_scroll(Scroll *s, int *pos, int *y_position)
{
  //erase_scr(fd_boxdbg, "\033[2J");
  TTYINTFD2(fd_boxdbg, *y_position, 1, s->pos_upper_t); ++*y_position;
  TTYINTFD2(fd_boxdbg, *y_position, 1, s->pos_lower_t); ++*y_position;
  TTYINTFD2(fd_boxdbg, *y_position, 1, s->n_lower_t); ++*y_position;
  TTYINTFD2(fd_boxdbg, *y_position, 1, s->array_size); ++*y_position;
  TTYINTFD2(fd_boxdbg, *y_position, 1, *pos); ++*y_position;
}

void open_file(STAT_INFO *info)
{
  info->file = fopen(info->file_name, "rb");
  if (info->file == NULL) {
    fprintf(stderr, "Error opening file: %s\n", info->file_name);
    exit(1);
  }
  fseek(info->file, 0, SEEK_END);
  info->file_len = ftell(info->file);
  printf("%lu", info->file_len);
  fseek(info->file, 0, SEEK_SET);
}

char find_file_type(STAT_INFO *info, char *file_name)
{
  info->file = fopen(file_name, "rb");
  if (info->file == NULL) {
    fprintf(stderr, "Error opening file: %s\n", file_name);
    exit(1);
  }

  fseek(info->file, 0, SEEK_END);
  info->file_len = ftell(info->file);

  char *file_type = NULL;
  char type = 0;

  fseek(info->file, 0, SEEK_SET);
  unsigned char buffer[16];
  fread(buffer, 16, 1, info->file);

  size_t i;
  if (buffer[0] == SIGNATURE_JPG[0]) {
    for (i = 1; i < 3; ++i) { // 0xff 0xd8 0xff
      if (buffer[i] != SIGNATURE_JPG[i]) {
        goto quit_file_type;
      }
    }
    type = *TYPE[0];
  } else if (buffer[0] == SIGNATURE_PNG[0]) {
    fseek(info->file, 0, SEEK_SET);
    for (i = 0; i < SIZE_PNG_ARRAY - 1; ++i) {
      if (buffer[i] != SIGNATURE_PNG[i]) {
        goto quit_file_type;
      }
    }
    type = *TYPE[1];
  } else if (buffer[0] == SIGNATURE_GIF[0]) {
    for (i = 1; i < SIZE_GIF_ARRAY; ++i) {
      if (buffer[i] != SIGNATURE_GIF[i]) {
        goto quit_file_type;
      }
    }
    type = *TYPE[2];
  }
quit_file_type:
  fclose(info->file);
  return type;
}


int print_right_window3(Array **left_box,
                        Array **right_box,
                        Scroll *s,
                        Window_ *w1,
                        Window_ *w2,
                        Window_ *w_main,
                        Message *msg,
                        STAT_INFO *info_file,
                        int pos, int *c)
{
  if (pos >= 0 && (*left_box)->n_elements > 0 && pos < (*left_box)->n_elements && *((*left_box)->menu[pos].type) == 'd') {
    if ((*right_box)->n_elements != 0) {
      free_array(*right_box);
      //free_array2(right_box);
      //initialize_array(right_box, 1);
      initialize_array2(&right_box, 1);
    }
    parcours((*left_box)->menu[pos].complete_path, 0, *right_box, 0, w_main);
    int to_print = 0;
    if ((*right_box)->n_elements <= s->n_to_print) {
      to_print = (*right_box)->n_elements;
    } else {
      if ((*right_box)->n_elements >= w2->y_size - 1) {
        to_print = w2->y_size - 1;
      } else {
        to_print = (*right_box)->n_elements;
      }
    }
    size_t i;
    for (i = 0; i < to_print; ++i) {
      mvwprintw(w2, *right_box, i + w2->y_beg + 1, w2->x_beg + 1, (*right_box)->menu[i].name, i);
    }
    mv(pos - s->pos_upper_t + w1->y_beg + 1, w1->x_beg + 1);

  } else if (pos >= 0 && (*left_box)->n_elements > 0 && pos < (*left_box)->n_elements &&
      (match_extension((*left_box)->menu[pos].name, "gz") ||
       match_extension((*left_box)->menu[pos].name, "xz"))) {
    read_tar(*left_box, &pos);
    mv(pos - s->pos_upper_t + w1->y_beg + 1, w1->x_beg + 1);
  } else if ((*left_box)->n_elements > 0 && *((*left_box)->menu[pos].type) == 'f') {
      char file_type = find_file_type(info_file, (*left_box)->menu[pos].complete_path);
      if (file_type == 'j' || file_type == 'p' || file_type == 'g') {
        sem_wait(&mutex);
        info_key_presses.last_position_array = pos;
        //ttymode_reset(ECHO, 0);
        if (*((*left_box)->menu[pos > 0 ? pos - 1 : pos].type) == 'd') {
          info_key_presses.last_element_is_not_img = 1;
        }
        int bytes_read = 4096;
        char buffer[bytes_read];
        int total_read = 0;
        pid_t pid;
        if (resized == 0) {
          ttymode_reset(ECHO, 1);
          print_tty_filesize3(2, w_main->y_size - 3, ((double)info_file->file_len / 1000.0));
          mv(pos - s->pos_upper_t + w1->y_beg + 1, w1->x_beg + 1);
          ttymode_reset(ECHO, 0);
          total_read = show_image(&pid, buffer, &bytes_read, (*left_box)->menu[pos].complete_path);
#if defined(PRINT_OTHERTTY)
          //TTYINT(1, 1, total_read);
#endif // PRINT_OTHERTTY
          ttymode_reset(ECHO, 1);
          mv(w_main->y_size - 3, 2);
          empty_space_debug(20);
          mv(pos - s->pos_upper_t + w1->y_beg + 1, w1->x_beg + 1);
          ttymode_reset(ECHO, 0);
          if (resized == 0) {
            *c = atoi(buffer);
          }
          image_used = 1;
        }
        image_appeared = 1;
        sprintf(position, place_, pos - s->pos_upper_t + w1->y_beg + 1, w1->x_beg + 1);
        move(1, position);
        sem_post(&mutex);
        return 1;
      } else if (pos >= 0 && (*left_box)->n_elements > 0 &&
                 match_extension((*left_box)->menu[pos].name, ".c")       ||
                 match_extension((*left_box)->menu[pos].name, ".cpp")     ||
                 match_extension((*left_box)->menu[pos].name, ".h")       ||
                 match_extension((*left_box)->menu[pos].name, ".java")    ||
                 match_extension((*left_box)->menu[pos].name, ".txt")     ||
                 match_extension((*left_box)->menu[pos].name, ".md")      ||
                 match_extension((*left_box)->menu[pos].name, ".py")      ||
                 match_extension((*left_box)->menu[pos].name, ".sh")      ||
                 match_extension((*left_box)->menu[pos].name, ".json")    ||
                 match_extension((*left_box)->menu[pos].name, ".patch")   ||
                 match_extension((*left_box)->menu[pos].name, "Makefile") ||
                 match_extension((*left_box)->menu[pos].name, ".cmake")) {

        read_file2(*left_box, w1, w2, s, pos);
      }
  }
  return 0;
}

void ask_user(char *warning, int *c)
{
  write_len(warning);
  ttymode_reset(ECHO, 1);
  if ((*c = kbget()) == 'Y') {
    char *affirm = "Y";
    write_len(affirm);
    delete_file_folder_request = 1;
  } else if (*c == 'n') {
    char *negative = "n";
    write_len(negative);
    delete_file_folder_request = 0;
  }
  ttymode_reset(ECHO, 0);
}

void scroll_window_up7(Window_ *w, Array *box, Scroll *s, int *pos)
{
  int i, j = *pos, k = 0;
  int scroll_up_value = 5;
  int n_to_print = 0;
  const int last_element_pos = box->n_elements - 1;
  if (box->n_elements > w->y_size - 1) {
    n_to_print = w->y_size - 1;
  } else {
    n_to_print = box->n_elements;
  }
  if (*pos > 0 &&
       *pos - scroll_up_value >= 0 &&
        *pos <= s->pos_lower_t &&
         *pos - scroll_up_value >= scroll_up_value  &&
           s->pos_upper_t != 0 && s->pos_lower_t != 0 ||
          *pos == s->pos_upper_t) {

    if (*pos == s->pos_lower_t && s->pos_upper_t == 0) {
      int pos_to_highlight = *pos - scroll_up_value;
      for (int d = *pos; d >= pos_to_highlight + 1; --d) {
        if (*pos > 0) {
          mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
          print(w, box, *pos);
          --*pos;
          if (*pos >= 0) {
            mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
            highlight4(w, box, pos);
            usleep(50000);
          }
        }
      }
    } else {
      erase_window(w, s);
      int n = *pos - s->pos_upper_t + w->y_beg + 1;
      int up = *pos - s->pos_upper_t;
      int dn = s->pos_lower_t - *pos;
      int a = 0;
      for (int t = 0; t < scroll_up_value; --*pos, --s->pos_upper_t, --s->pos_lower_t, ++s->n_lower_t, --s->n_upper_t, ++t) {
        if (*pos >= 1 && *pos < box->n_elements - 1 && s->pos_upper_t > 0) {
          for (a = 0; a < up; ++a) {
            mverase(a + w->y_beg + 1, w->x_beg + 1, w->x_size);
            if (s->pos_upper_t + a - 1 >= 0) {
              print(w, box, s->pos_upper_t + a - 1);
            }
          }
          mverase(a + w->y_beg + 1, w->x_beg + 1, w->x_size);
          int orig = *pos - 1;
          highlight4(w, box, &orig);
          usleep(50000);
          for (++a; a < dn + up + 1; ++a) {
            mverase(a + w->y_beg + 1, w->x_beg + 1, w->x_size);
            if (s->pos_upper_t + a - 1 >= 0) {
              print(w, box, s->pos_upper_t + a - 1);
            }
          }
        } else if (s->pos_upper_t == 0) {
          for (int g = 0; g < (scroll_up_value - t); ++g) {
            mverase(*pos - s->pos_upper_t + w->y_beg + 1 - g, w->x_beg + 1, w->x_size);
            print(w, box, *pos);
            if (g == scroll_up_value - t - 1) {
              mverase(*pos - 1 - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
              int orig2 = *pos - 1;
              highlight4(w, box, &orig2);
            }
          }
        }
        if (s->pos_upper_t < 0) {
            mverase(*pos + w->y_beg + 1, w->x_beg + 1, w->x_size);
            print(w, box, *pos);
            if (s->pos_upper_t < 0) {
              mverase(*pos + w->y_beg, w->x_beg + 1, w->x_size);
              int orig2 = *pos - 1;
              highlight4(w, box, &orig2);
            }
            usleep(50000);
            ++n;
        }
      }
    }
  } else if (s->pos_upper_t == 0) {
    int pos_to_highlight = *pos - scroll_up_value;
    for (int d = *pos; d >= pos_to_highlight + 1; --d) {
      if (*pos > 0) {
        mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
        print(w, box, *pos);
        --*pos;
        if (*pos >= 0) {
          mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
          highlight4(w, box, pos);
          usleep(50000);
        }
      }
    }
  } else if (*pos - scroll_up_value < scroll_up_value) {
    int a;
    int up = *pos - s->pos_upper_t;
    int dn = s->pos_lower_t - *pos;
    for (int i = 0; i < scroll_up_value && *pos >= 0; ++i, --*pos, --s->pos_lower_t, --s->pos_upper_t, ++s->n_lower_t) {

      if (*pos >= 0) {
        for (a = 0; a < up; ++a) {
          mverase(a + w->y_beg + 1, w->x_beg + 1, w->x_size);
          if (s->pos_upper_t + a - 1 >= 0) {
            print(w, box, s->pos_upper_t + a - 1);
          }
        }
        mverase(a + w->y_beg + 1, w->x_beg + 1, w->x_size);
        int orig = *pos - 1;
        highlight4(w, box, &orig);
        usleep(50000);
        for (++a; a < dn + up + 1; ++a) {
          mverase(a + w->y_beg + 1, w->x_beg + 1, w->x_size);
          if (s->pos_upper_t + a - 1 >= 0) {
            print(w, box, s->pos_upper_t + a - 1);
          }
        }
      }
    }
  }

  while (s->pos_upper_t <= -1) {
    ++s->pos_upper_t;
    ++s->pos_lower_t;
    --s->n_lower_t;
  }
}

void scroll_window5(Window_ *w, Array *box, Scroll *s, int *pos)
{
  int i, j, k, n_to_scroll = 0;
  int scroll_down_value = 5;
  int n_to_print = 0;
  const int last_element_pos = box->n_elements - 1;
  if (box->n_elements > w->y_size - 1) {
    n_to_print = w->y_size - 1;
  } else {
    n_to_print = box->n_elements;
  }
  erase_window(w, s);
  for (; s->n_lower_t >= 0 && n_to_scroll < scroll_down_value; ++s->pos_lower_t, ++s->pos_upper_t, --s->n_lower_t, ++*pos, ++n_to_scroll) {
    for (k = 0; k < *pos - s->pos_upper_t && *pos < box->n_elements && k + s->pos_upper_t < box->n_elements - 1; ++k) {
      mverase(k + w->y_beg + 1, w->x_beg + 1, w->x_size);
      print(w, box, k + s->pos_upper_t);
    }
    for (i = 0, j = *pos; i < n_to_print - k && (j + 1 < box->n_elements); ++i) {
      mverase(i + *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
      if (i == 0) {
        highlight4(w, box, pos);
      } else {
        print(w, box, ++j);
      }
    }
    usleep(50000);
  }

  if (*pos - 1 >= 0) {
    --*pos;
  }
  --s->pos_upper_t;
  --s->pos_lower_t;
  ++s->n_lower_t;

  int left_to_scroll = scroll_down_value - n_to_scroll;
  if (left_to_scroll > 1) {
    mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
    print(w, box, *pos);

    for (int j_ = 0; j_ < left_to_scroll && j_ <= s->pos_lower_t && *pos <= s->pos_lower_t; ++*pos, ++j_) {
      mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
      highlight4(w, box, pos);
      usleep(50000);
      mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
      print(w, box, *pos);
    }
    if (*pos >= box->n_elements) {
      --*pos;
    }
    mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
    highlight4(w, box, pos);
  }

#if defined(BOXDBG)
    if (debug_fd_scroll_pos) {
      int y_position = 1; //_PRINTDEBUG;
      TTYINTFD2(fd_boxdbg, y_position, 1, *pos + scroll_down_value); ++y_position;
      TTYINTFD2(fd_boxdbg, y_position, 1, *pos - s->pos_upper_t + w->y_beg + 1); ++y_position;
      TTYINTFD2(fd_boxdbg, y_position, 1, n_to_print - k); ++y_position;
      TTYINTFD2(fd_boxdbg, y_position, 1, n_to_print); ++y_position;
      TTYINTFD2(fd_boxdbg, y_position, 1, k); ++y_position;
      TTYINTFD2(fd_boxdbg, y_position, 1, w->y_size); ++y_position;
      TTYINTFD2(fd_boxdbg, y_position, 1, left_to_scroll); ++y_position;
    }
#endif // BOXDBG
}

void scroll_window4(Window_ *w, Array *box, Scroll *s, int *pos)
{
  int i, j, k, n_to_scroll = 0;
  int scroll_down_value = 5;
  int n_to_print = 0;
  const int last_element_pos = box->n_elements - 1;
  if (box->n_elements > w->y_size - 1) {
    n_to_print = w->y_size - 1;
  } else {
    n_to_print = box->n_elements;
  }
  erase_window(w, s);
  for (; s->n_lower_t >= 0 && n_to_scroll < scroll_down_value; ++s->pos_lower_t, ++s->pos_upper_t, --s->n_lower_t, ++*pos, ++n_to_scroll) {
    for (k = 0; k < *pos - s->pos_upper_t; ++k) {
      if (*pos < box->n_elements && k + s->pos_upper_t < box->n_elements - 1) {
        mverase(k + w->y_beg + 1, w->x_beg + 1, w->x_size);
        print(w, box, k + s->pos_upper_t);
      }
    }
    for (i = 0, j = *pos; i < n_to_print - k; ++i) {
      if (*pos < box->n_elements && j + 1 < box->n_elements) {
        mverase(i + *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
        if (i == 0) {
          highlight4(w, box, pos);
        } else {
          print(w, box, ++j);
        }
      }
    }
    //--s->pos_lower_t; ++s->pos_upper_t; --s->n_lower_t; ++*pos; ++n_to_scroll;
    usleep(50000);
  }

  if (*pos - 1 >= 0) {
    --*pos;
  }
  --s->pos_upper_t;
  --s->pos_lower_t;
  ++s->n_lower_t;

#if defined(BOXDBG)
    if (debug_fd_scroll_pos) {
      int y_position = 1; //_PRINTDEBUG;
      TTYINTFD2(fd_boxdbg, y_position, 1, *pos + scroll_down_value); ++y_position;
      TTYINTFD2(fd_boxdbg, y_position, 1, *pos - s->pos_upper_t + w->y_beg + 1); ++y_position;
      TTYINTFD2(fd_boxdbg, y_position, 1, n_to_print - k); ++y_position;
      TTYINTFD2(fd_boxdbg, y_position, 1, n_to_print); ++y_position;
      TTYINTFD2(fd_boxdbg, y_position, 1, k); ++y_position;
      TTYINTFD2(fd_boxdbg, y_position, 1, w->y_size); ++y_position;
    }
#endif // BOXDBG
}

void scroll_window3(Window_ *w, Array *box, Scroll *s, int *pos)
{
  int i, j = *pos, k = 0, j_2 = 0;
  int scroll_down_value = 5;
  int n_to_print = 0;
  const int last_element_pos = box->n_elements - 1;
  if (box->n_elements > w->y_size - 1) {
    n_to_print = w->y_size - 1;
  } else {
    n_to_print = box->n_elements;
  }
  if (*pos < s->array_size - 1 &&
       *pos + scroll_down_value <= last_element_pos &&
        *pos >= s->pos_upper_t && s->pos_lower_t != box->n_elements - 1 &&
         *pos + scroll_down_value <= last_element_pos - scroll_down_value ||
          *pos == s->pos_lower_t &&
          s->pos_lower_t != last_element_pos /*|| (*pos == s->pos_lower_t && s->pos_lower_t != last_element_pos)*/ //&&
          /*s->pos_lower_t > 0*/) {
    erase_window(w, s);
    int old_lower_pos = s->pos_lower_t;
    for (; j_2 < scroll_down_value && s->n_lower_t >= 0; ++*pos, ++s->pos_upper_t, ++s->pos_lower_t, --s->n_lower_t, ++j_2) {
      for (k = 0; k < *pos - s->pos_upper_t; ++k) {
        if (*pos < box->n_elements && k + s->pos_upper_t < box->n_elements - 1) {
          mverase(k + w->y_beg + 1, w->x_beg + 1, w->x_size);
          print(w, box, k + s->pos_upper_t);
        }
      }
      for (i = 0, j = *pos; i < n_to_print - k; ++i) {
        if (*pos < box->n_elements && j + 1 < box->n_elements) {
          mverase(i + *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
          if (i == 0) {
            highlight4(w, box, pos);
          } else {
            print(w, box, ++j);
          }
        }
      }
      usleep(50000);
    }
#if defined(BOXDBG)
    if (debug_fd_scroll_pos) {
      int y_position = 1; //_PRINTDEBUG;
      TTYINTFD2(fd_boxdbg, y_position, 1, *pos + scroll_down_value); ++y_position;
      TTYINTFD2(fd_boxdbg, y_position, 1, *pos - s->pos_upper_t + w->y_beg + 1); ++y_position;
      TTYINTFD2(fd_boxdbg, y_position, 1, n_to_print - k); ++y_position;
      TTYINTFD2(fd_boxdbg, y_position, 1, n_to_print); ++y_position;
      TTYINTFD2(fd_boxdbg, y_position, 1, k); ++y_position;
      TTYINTFD2(fd_boxdbg, y_position, 1, w->y_size); ++y_position;
    }
#endif // BOXDBG

    if (*pos - 1 >= 0) {
      --*pos;
    }
    --s->pos_upper_t;
    --s->pos_lower_t;
    ++s->n_lower_t;
  } else if (*pos + scroll_down_value > last_element_pos) {
      int orig_pos = *pos;
      int i = 0;
      mverase(i + *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
      print(w, box, *pos);
      for (i = 0; i < (s->pos_lower_t - orig_pos); ++i, ++*pos) {
        mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
        highlight4(w, box, pos);
        usleep(50000);
        mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
        print(w, box, *pos);
      }
      if (*pos == box->n_elements - 2) {
        mverase((*pos) - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
        int v_pos = *pos + 1;
        highlight4(w, box, &v_pos);
      } else {
        mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
        highlight4(w, box, pos);
      }
    }
    //else if (*pos == s->pos_lower_t && s->pos_lower_t == box->n_elements - 1) {
    //  return;
    //}
    else if (*pos < s->pos_lower_t - 1) {

      //
      if (s->n_lower_t < n_to_print && s->pos_lower_t != last_element_pos) {
        int saved_pos = s->pos_lower_t - *pos;
        for (int i = 0; i < saved_pos; ++i, ++*pos) {
          mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
          highlight4(w, box, pos);
          usleep(50000);
          mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
          print(w, box, *pos);
        }
        erase_window(w, s);
        s->pos_lower_t = last_element_pos;
        s->pos_upper_t = s->pos_lower_t - n_to_print + 1;
        s->n_lower_t = 0;
        ++*pos;
        for (int d = s->pos_upper_t; d <= s->pos_lower_t; ++d) {
          mverase(d - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
          print(w, box, d);
        }
        for (int r = 0; r <= scroll_down_value - saved_pos; ++r) {
          mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
          highlight4(w, box, pos);
          usleep(50000);
          if (r != scroll_down_value - saved_pos) {
            mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
            print(w, box, *pos);
            ++*pos;
          }
        }
      }
      //
      else {
        mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
        print(w, box, *pos);

        for (int j_ = 0; j_ < scroll_down_value; ++*pos, ++j_) {
          if (*pos < box->n_elements) {
            mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
            highlight4(w, box, pos);
            usleep(50000);
            mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
            print(w, box, *pos);
          }
        }
        mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
        highlight4(w, box, pos);
      }
    } else {
      erase_window(w, s);
      int pos_to_highlight = scroll_down_value - (s->pos_lower_t - *pos);
      s->pos_lower_t = last_element_pos;
      s->pos_upper_t = s->pos_lower_t - n_to_print + 1;
      s->n_lower_t = 0;
      //si space ds mv backslash plein de problemes
      *pos = s->pos_upper_t;
      // reprint
      int d;
      for (d = s->pos_upper_t; d <= s->pos_lower_t; ++d) {
        mverase(d - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
        print(w, box, d);
      }

      int r, t = 1;
      for (r = 0; r <= pos_to_highlight; ++r) {
        mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
        highlight4(w, box, pos);
        usleep(50000);
        if (r != pos_to_highlight) {
          mverase(*pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1, w->x_size);
          print(w, box, *pos);
          ++*pos;
        }
      }
    }
}

int show_image(pid_t *pid, char *buffer, int *bytes_read, char *img_path)
{
  int total_read = 0;
  int pfd[2];
  pipe(pfd);
// https://stackoverflow.com/questions/8257714/how-to-convert-an-int-to-string-in-c
#define ENOUGH_DOUBLE ((CHAR_BIT * sizeof(double)) / 3 + 2)
#define STRINGFROMINT "%.2f"
  char win_top_limit[ENOUGH_DOUBLE];
  char win_lower_limit[ENOUGH_DOUBLE];
  sprintf(win_top_limit, STRINGFROMINT, (double)(w1.y_size + 40));
  sprintf(win_lower_limit, STRINGFROMINT, 210.0);
#if defined(EBUG)
  write(fd, win_top_limit, strlen(win_lower_limit)); write(fd, "\n", strlen("\n"));
  write(fd, win_lower_limit, strlen(win_lower_limit));
#endif // EBUG

  int capacity = *bytes_read;

  int left_offset = 0;
  if (number_of_windows > 2) {
    left_offset = 3;
  }
  char offset_left[ENOUGH_INT];
  sprintf(offset_left, NUMINTEGER, number_of_windows);
  if ((*pid = fork()) == 0) {
    close(pfd[0]);                      // child doesn't need read pipe
    dup2(pfd[1], STDOUT_FILENO);        // insure write pipe is at stdout (fd#1)
    dup2(pfd[1], STDERR_FILENO);        // stderr goes to the pipe also (optional)
    close(pfd[1]);                      // child doesn't need to write pipe any more
    //execl("./draw", "./draw", win_top_limit, offset_left, win_lower_limit, img_path, (char *)0);
    execl("draw", "draw", win_top_limit, offset_left, win_lower_limit, img_path, (char *)0);
    _exit(1);
  } else {
    close(pfd[1]);                      // parent doesn't need write pipe
    while ((*bytes_read = read(pfd[0], &buffer[total_read], capacity - total_read)) > 0)
      total_read += *bytes_read;
    if (total_read > 0)
      buffer[total_read] = '\0';
    wait(NULL);
  }
  return total_read;
}

int strpos4(char *hay, char *needle, int offset)
{
  int len_hay = strlen(hay);
  if (len_hay > 0) {
    for (int i = 0; i < len_hay; ++i) {
      if (hay[i + offset] == *needle) {
        return i + offset;
      }
    }
  }
  return -1;
}

void read_file2(Array *left_box, Window_ *w1, Window_ *w2, Scroll *s, int pos)
{
  FILE * fp;
  char *read_line = NULL;
  //char read_line[4096];
  size_t len = 0;
  ssize_t read;


  //MALLOC(read_line, 4096);

  fp = fopen(left_box->menu[pos].complete_path, "r");
  if (fp == NULL) {
    PRINT("read file error");
  }

  sprintf(position, place_, w2->y_beg + 2, w2->x_beg + 1);
  move(1, position);
  int n_lines = 0;
  while ((read = getline(&read_line, &len, fp)) != -1) {
    if (n_lines < w2->y_size - 1) {
      int len = strlen(read_line);
      sprintf(position, place_, w2->y_beg + n_lines + 1, w2->x_beg + 1);
      move(1, position);
      del_from_cursor(del_in);
      move(1, position);
      // goes past window limits if first characters are spaces
      int pos_tab = strpos4(read_line, "\t", 0);
      char *copy_read = NULL;
      int counter = 0;
      if (pos_tab >= 0) {
        copy(&copy_read, read_line, len);
        do {
          ++counter;
          copy_read[pos_tab] = ' ';
          pos_tab = strpos4(copy_read, "\t", pos_tab);
        //} while (pos_tab > -1 /* && pos_tab < (w2->x_size - 2 - counter) && read_line[pos_tab] == '\t' */ && pos_tab < len  && len > 0);
        } while (pos_tab > -1 && read_line[pos_tab] == '\t' && pos_tab < len && len > 0);
        ++counter;
      }
      if (len > (w2->x_size) - 2) {
        len = (w2->x_size) - 2 - counter;
      }
      if (read_line[len - 1] == '*' || read_line[len - 2] == '*') {
        write(1, read_line, len - 1);
      } else if (copy_read != NULL) {
        write(1, copy_read, len);
        free(copy_read);
        copy_read = NULL;
      } else {
        write(1, read_line, len);
      }
      ++n_lines;
    }
  }

  fclose(fp);
  if (read_line) {
    free(read_line);
    read_line = NULL;
  }

  sprintf(position, place_, pos - s->pos_upper_t + w1->y_beg + 1, w1->x_beg + 1);
  move(1, position);

}

void print_in_char(char *msg, ...)
{
  va_list args = { 0 };
  va_start(args, msg);
  vfprintf(stderr, msg, args);
  va_end(args);
}

#define CONCAT(a, b) CONCAT(a, b)
#define CONCAT_(a, b) a ## b

// Concatenate preprocessor tokens A and B without expanding macro definitions
// (however, if invoked from a macro, macro arguments are expanded).
#define PPCAT_NX(A, B) A ## B
// Concatenate preprocessor tokens A and B after macro-expanding them.
#define PPCAT(A, B) PPCAT_NX(A, B)
// Turn A into a string literal without expanding macro definitions
// (however, if invoked from a macro, macro arguments are expanded).
#define STRINGIZE_NX(A) #A
// Turn A into a string literal after macro-expanding it.
#define STRINGIZE(A) STRINGIZE_NX(A)

void print_message2(Window_ *w, Scroll *s, int position_from_end_scr, int pos, Message *msg)
{
  snprintf(position, strlen(place_), place_, w->y_size - position_from_end_scr, w->x_beg + 1);
  move(1, position);
  del_from_cursor(del_in);
  if (msg->used_ulong) {
#ifndef value_return
#define value_return " %lu"

#endif
  char *nth = STRINGIZE(msg->print_msg);
  char *mth = STRINGIZE_NX(value_return);
  write(1, msg->print_msg, strlen(msg->print_msg));
  write(1, " ", strlen(" ")),
  print_in_char(STRINGIZE(PPCAT_NX(nth, STRINGIZE_NX(value_return))), msg->n_ulong);
  msg->used_ulong = 0;
  } else if (msg->used_int) {
#ifndef value_return
#define value_return "c = %d"
#endif
  print_in_char(value_return, msg->n_int);
  msg->used_int = 0;
  } else if (msg->used_uint) {
#ifndef value_return
#define value_return "c = %u"
#endif
  print_in_char(value_return, msg->n_uint);
  msg->used_uint = 0;
  } else if (msg->used_char) {
#ifndef value_return
#define value_return "c: %s"
#endif
  print_in_char(value_return, msg->n_char);
  msg->used_char = 0;
  }
#undef value_return
  snprintf(position, strlen(place_), place_, pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
  move(1, position);
}

void print_permissions(Array *a, Scroll *s1, Window_ *w, int pos)
{
/*
  printf("  %s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
      bg_blue, " NORMAL", bg_reset, bg_cyan,
      fg_blue, r_full_triangle, fg_reset, fg_blue, r_line_triangle, bg_reset, fg_reset, bg_light_blue, " ", bg_reset);
*/
  if (pos >= 0 && pos < a->n_elements -  1) {
    //mv(w_main.y_size - 1, w_main.x_beg + 1);
    //del_from_cursor(del_in);
    //mv(w_main.y_size - 2, w_main.x_beg + 1);
    //del_from_position(del_in);
    write_partial(a->menu[pos].permissions, strlen(a->menu[pos].permissions));
    //mv(pos - s1->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
  }
}

void print_n_elements(Array *left_box)
{
#define n_elements_left_box "%d"
  char N_ELEMENTS[sizeof(n_elements_left_box)];
  sprintf(N_ELEMENTS, n_elements_left_box, left_box->n_elements);
  sprintf(position, place_, w_main.y_size - 3, w_main.x_beg + 1);
  move(1, position);
  write(1, "n_elements: ", strlen("n_elements: "));
  del_from_cursor(del_in);
  write(1, "    ", strlen("    "));
  write(1, N_ELEMENTS, strlen(N_ELEMENTS));
}

void find_user(char *in, char **user)
{
  unsigned int i, counter = 0;
  for (i = 0; *(in + i) != '\0'; ++i) {
    if (in[i] == '/' && ++counter == 3) {
      break;
    }
  }
  copy(user, in, i + 1);
}

int mv_to_trash3(Window_ *w1, Scroll *s, Array **left_box, int *pos, int *option)
{
  size_t len_file = strlen((*left_box)->menu[*pos].complete_path);

  char *trash_folder2 = ".local/share/Trash/";
  size_t len_trash_folder2 = strlen(trash_folder2);
  char *trash_folder = NULL;
  char *user;
  find_user((*left_box)->menu[*pos].complete_path, &user);
  size_t len_user = strlen(user);

  combine(&trash_folder, user, trash_folder2, len_user, len_trash_folder2);
  char *_parent = NULL;
  get_parent((*left_box)->menu[*pos].complete_path, &_parent, 1);
  char *path_to_be_copied = NULL;

  size_t len_name = strlen((*left_box)->menu[*pos].name);
  size_t len_trash_folder_new = strlen(trash_folder);
  combine(&path_to_be_copied, trash_folder, (*left_box)->menu[*pos].name, len_trash_folder_new, len_name);
  if (*((*left_box)->menu[*pos].type) == 'f') {
    if (check_if_file_exists(trash_folder, path_to_be_copied, w1)) {
      unlink(path_to_be_copied);
    }
    if (cp(file_to_be_copied, path_to_be_copied) != 0) {
      return 0;
    }
    unlink((*left_box)->menu[*pos].complete_path);
    copy(&deleted_file, (*left_box)->menu[*pos].complete_path, len_file);
    //copy(&deleted_file, (*left_box)->menu[*pos].complete_path, len_name);
  } else if (*((*left_box)->menu[*pos].type) == 'd') {
    struct stat info;
    if (stat((*left_box)->menu[*pos].complete_path, &info) != 0) {
      // error
    } else {
      mkdir(path_to_be_copied, info.st_mode);
    }

    remove_files_from_folder((*left_box)->menu[*pos].complete_path, w1);
    remove((*left_box)->menu[*pos].complete_path);
  }

  if ((*left_box)->n_elements != 0) {
    free_array(*left_box);
    (*left_box)->n_elements = 0;
    initialize_array2(&left_box, 1);
  }
  erase_window(w1, s);
  parcours(_parent, 0, *left_box, 0, &w_main);
  if ((*left_box)->n_elements > 0 && *pos >= (*left_box)->n_elements - 1) {
    *pos = (*left_box)->n_elements - 1;
  } else if ((*left_box)->n_elements == 0) {
    *pos = 0;
  }
  reprint_menu(w1, s, *left_box, attributes, *pos, *option);

  if (_parent) {
    free(_parent);
    _parent = NULL;
  }
  if (trash_folder != NULL) {
    free(trash_folder);
    trash_folder = NULL;
  }
  if (user != NULL) {
    free(user);
    user = NULL;
  }
  if (path_to_be_copied != NULL) {
    free(path_to_be_copied);
    path_to_be_copied = NULL;
  }
  return 1;
}

void print_box_fd(int fd, Array *box, int *y_position)
{
  *y_position = 1;
  erase_scr(fd, "\033[2J");
  for (int i = 0; i < box->n_elements; ++i) {
    TTYSTRFD(fd, i + *y_position, 1, box->menu[i].complete_path);
  }
}

void print_all_attributes_fd(int fd, Attributes *attr, int *y_position)
{
  *y_position = 1;
  erase_scr(fd, "\033[2J");
  for (int i = 0; i < attr->n_elements; ++i) {
    TTYSTRFD(fd, *y_position, 1, attr->paths[i]); ++*y_position;
    TTYINTFD2(fd, *y_position, 1, attr->pos[i]->m_position); ++*y_position;
    TTYINTFD2(fd, *y_position, 1, attr->pos[i]->m_lower_pos); ++*y_position;
    TTYINTFD2(fd, *y_position, 1, attr->pos[i]->m_upper_pos); ++*y_position;
    TTYINTFD2(fd, *y_position, 1, attr->pos[i]->array_size); ++*y_position;
  }
}

int horizontal_navigation(int *c, int *pos, int *n_windows,
//int horizontal_navigation(long unsigned *c, int *pos, int *n_windows,
                          int *first_window_width_fraction,
                          Array **left_box, Array **right_box, Array **w0_left_box, Attributes **attributes, Attributes **w0_attributes,
                          Window_ *w0, Window_ *w1, Window_ *w2, Positions *posit,
                          int position_before_copying, int *previous_pos, Scroll *s, Scroll *w0_s, int *previous_pos_w0, int *pos_w0,
                          int *second_previous_c, int *previous_pos_c, int *option, int *secondary_loop,
                          int *left_allocation, int *backspace)
{
  //*c = get_char();
  //*c = kbget();

  // TTYINTFD(1, 30, 1, *c);
  //  TTYSTRFD(1, 30, 1, "KEY_ESCAPE");
    if (image_used == 0 && (*c = get_char()) && /*(*c = kbget()) == KEY_Q*/ *c == KEY_Q && resized == 0) {
      return 0;
    } else
      if (image_used == 0 && (*c == 'l' || *c == KEY_ENTER || *c == ENTER) &&
               *((*left_box)->menu[*pos].type) == 'd' &&
               (*right_box)->n_elements != 0) {

      if (position_before_copying_sig) {
        position_before_copying_sig = 0;
        //*pos = position_before_copying;
        if (position_before_copying < (*left_box)->n_elements) {
          *pos = position_before_copying;
        }
      }

      info_key_presses.n_times_pressed = 0;

      n_elements_to_erase = (*left_box)->n_elements;

      *previous_pos = *pos;
      *previous_pos_w0 = *pos_w0;
      erase_window(w1, s);
      if (number_of_windows == 3) {
        erase_window(w0, w0_s);
      }

      posit->m_position = *pos;
      posit->m_upper_pos = s->pos_upper_t;
      posit->m_lower_pos = s->pos_lower_t;
      posit->window_number = 2;
      add_attr(*attributes, posit, (*left_box)->menu[*pos].complete_path);

      if (number_of_windows == 3 && *((*left_box)->menu[*pos].type) == 'd') {  /*&& initial_pass_for_w0 == 1*/
        char *parent_left_box = NULL;
        *pos_w0 = *pos;
        posit->m_upper_pos = s->pos_upper_t;
        posit->m_lower_pos = s->pos_lower_t;
        posit->array_size = s->array_size;
        posit->m_position = *pos;
        posit->window_number = 1;
        w0_s->pos_lower_t = s->pos_lower_t;
        w0_s->pos_upper_t = s->pos_upper_t;
        w0_s->array_size = s->array_size;
        add_attr(*w0_attributes, posit, (*left_box)->menu[*pos].complete_path);

        if (parent_left_box != NULL) {
          free(parent_left_box);
          parent_left_box = NULL;
        }
#if defined(PRINT_OTHERTTY)
        int i;
        erase_scr(_file_descriptor_, "\033[2J");
        for (i = 0; i < (*w0_attributes)->n_elements; ++i) {
          TTYSTR2(y_pts, 1, (*w0_attributes)->paths[i]); ++y_pts;
          TTYINT(y_pts, 1, (*w0_attributes)->pos[i]->m_upper_pos); ++y_pts;
          TTYINT(y_pts, 1, (*w0_attributes)->pos[i]->m_lower_pos); ++y_pts;
          TTYINT(y_pts, 1, (*w0_attributes)->pos[i]->m_position); ++y_pts;
          TTYINT(y_pts, 1, (*w0_attributes)->pos[i]->array_size); ++y_pts;
        }
#endif // PRINT_OTHERTTY
      }
      print_path(s, (*left_box)->menu[*previous_pos].complete_path, *pos, 0);

      if (number_of_windows == 3) {
        if ((*w0_left_box)->n_elements > 0 && (*w0_left_box)->capacity > 1) {
          free_array(*w0_left_box);
          //free_array2(w0_left_box);
          (*w0_left_box)->n_elements = 0;
        }
      }
      if ((*left_box)->n_elements != 0) {
        if (number_of_windows == 3 /*&& initial_pass_for_w0 == 1*/) {
          // to pass to w0_left_box

          (*left_box)->menu[*pos].upper_pos = s->pos_upper_t;
          (*left_box)->menu[*pos].lower_pos = s->pos_lower_t;
          (*left_box)->menu[*pos].highlighted_pos = *pos;
          (*left_box)->menu[*pos].previous_pos = *pos;
          (*left_box)->menu[*pos].has_scroll = 1;
          if ((*left_box)->n_elements > 0) {
            //initialize_array(w0_left_box, (*left_box)->n_elements);
            initialize_array2(&w0_left_box, (*left_box)->n_elements);
          } else {
            //initialize_array(w0_left_box, 1);
            initialize_array2(&w0_left_box, 1);
          }
          dupArray2(*left_box, *w0_left_box);
        }

        free_array(*left_box);
        //free_array2(left_box);
        (*left_box)->n_elements = 0;
        if ((*right_box)->n_elements > 0) {
          //initialize_array(left_box, (*right_box)->n_elements);
          initialize_array2(&left_box, (*right_box)->n_elements);
        } else {
          //initialize_array(left_box, 1);
          initialize_array2(&left_box, 1);
        }
        (*right_box)->menu->has_scroll = 0;
      }
      dupArray2(*right_box, *left_box);

      n_elements_to_erase = (*right_box)->n_elements;

      erase_window(w2, s);
      if ((*right_box)->n_elements != 0) {
        free_array(*right_box);
        //free_array2(right_box);
        (*right_box)->n_elements = 0;
        //initialize_array(right_box, 1);
        initialize_array2(&right_box, 1);
      }
      if (number_of_windows == 3 && initial_pass_for_w0 == 0) {
        initial_pass_for_w0 = 1;
      }
      *pos = 0;
      *option = update(w1, s, pos, (*left_box)->n_elements);
      if (number_of_windows == 3) {
        update(w0, w0_s, pos_w0, (*w0_left_box)->n_elements);
      }
      *secondary_loop = 1;
      back_pressed = 0;
      ++enter_backspace;
      enter_pressed = 1;

#if defined(EBUG)
      print_n_elements(&left_box);
#endif // EBUG

    } else if (*c == 'h' || *c == KEY_BACKSPACE || *c == BACKSPACE) {

      n_elements_to_erase = (*left_box)->n_elements;

      *second_previous_c = *previous_pos_c;
      erase_window(w1, s);
      --enter_backspace;
      enter_pressed = 0;

      //if ((*left_box)->n_elements != 0 && strlen((*left_box)->menu[*pos].complete_path) != 1) {
      if (*pos >= 0 /*&& strlen((*left_box)->menu[*pos].complete_path) != 1*/) {
        // chercher le parent avec l'inode place en ordre decroissant

        if (number_of_windows == 3) {
          erase_window(w0, s);
        }
#if defined(PRINT_OTHERTTY_2)
        y_pts_2 = 14;
        TTYSTRFD(_file_descriptor_2, y_pts_2, 1, "positions_before_functions");  ++y_pts_2;
        TTYINTFD(_file_descriptor_2, y_pts_2, 1, *pos_w0); ++y_pts_2;
        TTYINTFD(_file_descriptor_2, y_pts_2, 1, *previous_pos_w0); ++y_pts_2;
        TTYINTFD(_file_descriptor_2, y_pts_2, 1, *pos); ++y_pts_2;
        TTYINTFD(_file_descriptor_2, y_pts_2, 1, *previous_pos); ++y_pts_2;
#endif // PRINT_OTHERTTY_2
        getBackSpaceFolder6(left_box, w1, pos, previous_pos, s);
        if (number_of_windows == 3) {
          getBackSpaceFolder6(w0_left_box, w0, pos_w0, previous_pos_w0, w0_s);
        }
#if defined(PRINT_OTHERTTY_2)
        //y_pts_2 = 16;
        mv_debug_fd(_file_descriptor_2, y_pts_2, 1);
        _PRINTDEBUG;
        TTYSTRFD(_file_descriptor_2, y_pts_2, 1, "positions after functions"); ++y_pts_2;
        print_all_attributes(*w0_attributes, &y_pts_2); ++y_pts_2;
        TTYINTFD(_file_descriptor_2, y_pts_2, 1, *pos_w0); ++y_pts_2;
        TTYINTFD(_file_descriptor_2, y_pts_2, 1, *previous_pos_w0); ++y_pts_2;
        TTYINTFD(_file_descriptor_2, y_pts_2, 1, *pos); ++y_pts_2;
        TTYINTFD(_file_descriptor_2, y_pts_2, 1, *previous_pos); ++y_pts_2;
#endif // PRINT_OTHERTTY_2
      }

      erase_window(w2, s);

      if ((*right_box)->menu[0].complete_path != NULL && (*right_box)->n_elements != 0) {
        free_array(*right_box);
        //free_array2(right_box);
        (*right_box)->n_elements = 0;
        if ((*left_box)->n_elements != 0) {
          //initialize_array(right_box, (*left_box)->n_elements);
          initialize_array2(&right_box, (*left_box)->n_elements);
        }
      }

      *option = update(w1, s, pos, (*left_box)->n_elements);
      if (number_of_windows == 3) {
        update(w0, w0_s, pos_w0, (*w0_left_box)->n_elements);
      }
      *left_allocation = 0;
      *secondary_loop = 1;
      *backspace = 1;
      back_pressed = 1;

    } else if (*c == 'a') {
      *n_windows = 3;
      number_of_windows = 3;
      char *parent = NULL;
      //get_parent_nwindows((*left_box)->menu[*pos].complete_path, &parent);
      getParent((*left_box)->menu[*pos].complete_path, &parent);
      if ((*w0_left_box)->n_elements > 0) {
        free_array(*w0_left_box);
        (*w0_left_box)->n_elements = 0;
        initialize_array2(&w0_left_box, 1);
      }
      parcours(parent, 0, *w0_left_box, 0, w0);
      posit->m_position = 0;
      posit->m_upper_pos = 0;
      posit->m_lower_pos = 0;
      //posit->array_size = (*w0_left_box)->n_elements;
      posit->array_size = 1;

      posit->window_number = 0;

      //if ((*w0_attributes)->n_elements > 0) {
      //  free_attr(*w0_attributes);
      //  initialize_attr(w0_attributes, 1);
      //}
      add_attr(*w0_attributes, posit, parent);
      if (parent != NULL) {
        free(parent);
        parent = NULL;
      }
    } else if (*c == 'r') {
      *n_windows = 2;
      number_of_windows = 2;

      free_attr(*w0_attributes);
      initialize_attr(w0_attributes, 1);
      w0_s->array_size = 0;
      w0_s->pos_lower_t = 0;
      w0_s->pos_upper_t = 0;
      w0_s->n_lower_t = 0;
      w0_s->n_to_print = 0;
    } else if (*c == KEY_ESCAPE) {
      mode_normal = 0;
      mode_visual = 1;
    }
    *previous_pos_c = *c;

    //reprint_menu(w1, s, *left_box, *attributes, *pos, *option);

#if defined(BOXDBG)
    if (debug_c_pos) {
      int y_position = 1;
      TTYINTFD2(fd_boxdbg, y_position, 1, *c); ++y_position;
    }
#endif // BOXDBG
    return 1;
}

int window_resize2(Window_ *w_main, Window_ *w0, Window_ *w1, Window_ *w2,
                  Array *left_box, int n_windows, int *previous_val_n_windows,
                  int first_window_width_fraction,
                  struct winsize *w_s,
                  Scroll *s,
                  int *pos, int *initial_loop, int *option, int *i)
{
  w_main->y_size = w_s->ws_row;
  w_main->x_size = w_s->ws_col;

  unsigned int x_window_offset = 3;

  unsigned int total_window_width_taken = 0;

  double total_window_width = (double)w_main->x_size;
  double shared_width_between_windows = total_window_width * (1 - (1 / (n_windows + 1)));
  if (n_windows == 3) {
    w0->y_beg = 3;
    w0->x_beg = 1;
    w0->y_size = w_main->y_size - w0->y_beg - 5;
    w0->x_size = (w_main->x_size / (n_windows + 1));
    total_window_width_taken += w0->x_size;
  }

  w1->y_beg = 3;
  w1->y_size = w_main->y_size - w1->y_beg - 5;

  if (n_windows == 2) {
    w1->x_beg = 1;
    w1->x_size = w_main->x_size / n_windows;
    total_window_width_taken = w1->x_size;
  } else if (n_windows == 3) {
    w1->x_beg = w0->x_size % 2 == 0 ? w0->x_size : (w0->x_size + 1);
    w1->x_size = (shared_width_between_windows / 2.0) - (w0->x_size / 2);
    total_window_width_taken = w1->x_size;
  }

  w2->y_beg = 3;
  w2->y_size = w1->y_size;
  if (n_windows == 2) {
    w2->x_beg = w1->x_size;
    w2->x_size = w_main->x_size / n_windows;
    total_window_width_taken = w2->x_size;
  } else if (n_windows == 3) {
    w2->x_beg = w0->x_size + w1->x_size;
    w2->x_size = (shared_width_between_windows / 2.0) - (w0->x_size / 2);
    total_window_width_taken = w2->x_size;
  }

  w2->y_px_size = w_s->ws_ypixel;
  w2->x_px_size = w_s->ws_xpixel;
  sprintf(del_in, del, w1->x_size - 2);

  if (w1->y_size > w1->y_previous || w1->x_size > w1->x_previous ||
      w1->y_size < w1->y_previous || w1->x_size < w1->x_previous || n_windows != *previous_val_n_windows) {
    erase_scr(1, "\033[2J");

    if (n_windows == 3) {
      draw_box(w0);
    }
    draw_box(w1);
    draw_box(w2);
    //i++;

    *option = update(w1, s, pos, left_box->n_elements);

    if (*initial_loop) {
      mv(w1->y_beg + 1, w1->x_beg + 1);
      highlight4(w1, left_box, pos);
      for (*i = 1; *i < s->n_to_print; ++*i) {
        mvwprintw(w1, left_box, *i + w1->y_beg + 1, w1->x_beg + 1, left_box->menu[*i].name, *i);
      }
    }
    *previous_val_n_windows = n_windows;

    if (*initial_loop) {
      char *parent;
      get_parent(left_box->menu[*pos].complete_path, &parent, 1);
      print_path(s, parent, *pos, 0);
      if (parent != NULL) {
        free(parent);
        parent = NULL;
      }
    } else {
      print_path(s, left_box->menu[*pos].complete_path, *pos, 0);
    }

    resized = 1;
  }
  w1->y_previous = w1->y_size;
  w1->x_previous = w1->x_size;

  return 1;
}

void initialize_sigwinch(struct sigaction *sact)
{
  sigemptyset(&sact->sa_mask);
  sact->sa_flags = 0;
  sact->sa_handler = sig_win_ch_handler;
  char err1[] = "sigaction";
  if (sigaction(SIGWINCH, sact, NULL)) {
    restore_config;
    write(2, err1, sizeof(err1));
  }
}

void initialize_windows(Window_ *w_main, Window_ *w0, Window_ *w1, Window_ *w2)
{
  w_main->y_beg = 1;
  w_main->x_beg = 1;
  w_main->y_previous = 0;
  w_main->x_previous = 0;

  w1->y_beg = 3;
  w1->x_beg = 1;
  w1->y_previous = 0;
  w1->x_previous = 0;

  w2->y_beg = w1->y_beg;
  w2->x_beg = (w_main->x_size / 2);

  w0->y_beg = 1;
  w0->x_beg = 1;
}

int directory_placement2(Array *left_box, Array **right_box, Scroll *s, int *pos, Window_ *w1, Window_ *w2, Window_ *w_main)
{
  if ((*right_box)->n_elements != 0) {
    free_array(*right_box);
    init(*right_box, 1);
  }
  parcours(left_box->menu[*pos].complete_path, 0, *right_box, 0, w_main);
  int to_print = 0;
  if ((*right_box)->n_elements <= s->n_to_print) {
    to_print = (*right_box)->n_elements;
  } else {
    if ((*right_box)->n_elements >= w2->y_size - 1) {
      to_print = w2->y_size - 1;
    } else {
      to_print = (*right_box)->n_elements;
    }
  }
  size_t i;
  for (i = 0; i < to_print; ++i) {
    mvwprintw(w2, *right_box, i + w2->y_beg + 1, w2->x_beg + 1, (*right_box)->menu[i].name, i);
  }
  sprintf(position, place_, *pos - s->pos_upper_t + w1->y_beg + 1, w1->x_beg + 1);
  move(1, position);
  return 1;
}

void find_parent(char *parent, char **real_parent)
{
  size_t len_parent = strlen(parent);
  size_t k = len_parent - 1;
  for (; k >= 0; --k) if (parent[k] == '/') break;
  copy(real_parent, parent, k);
}

void get_first_parent(Array **w0_left_box, int *pos_w0, char **parent_w0_out)
{
  size_t length_parent_w0 = strlen((*w0_left_box)->menu[*pos_w0].complete_path);
  int p = length_parent_w0 - 1;
  if (length_parent_w0 != 1) {
    for (; p >= 0; --p) if ((*w0_left_box)->menu[*pos_w0].complete_path[p] == '/') break;
  }
  copy(parent_w0_out, (*w0_left_box)->menu[*pos_w0].complete_path, p);
}

int getBackSpaceFolder6(Array **left_box, Window_ *w, int *pos, int *previous_pos, Scroll *s)
{
  char *parent = NULL;
  size_t length_parent = 0;
  char *reference_path_for_parent = NULL;
  if ((*left_box)->n_elements == 0) {
    length_parent = strlen(deleted_file);
    reference_path_for_parent = deleted_file;
  } else {
    length_parent = strlen((*left_box)->menu[*pos].complete_path);
    reference_path_for_parent = (*left_box)->menu[*pos].complete_path;
  }
  size_t p = length_parent - 1, counter = 0;
  for (; p >= 0; --p) if (reference_path_for_parent[p] == '/' && ++counter == 2) break;
  if (p < length_parent - 1 && length_parent != 1) {
    if (length_parent != 1) {
      for (; p >= 0; --p) if (reference_path_for_parent[p] == '/') break;
    }
    copy(&parent, reference_path_for_parent, p);
    char *r_parent = NULL;
    int b = length_parent - 1;
    for (; b >= 0; --b) { if (reference_path_for_parent[b] == '/') { break; } }
    copy(&r_parent, reference_path_for_parent, b);
    free_array(*left_box);
    //free_array2(left_box);
    //free_array3(&left_box);
    (*left_box)->n_elements = 0;
    initialize_array2(&left_box, 1);

    char *real_parent = NULL;
    if (file_pasted_signal) {
      size_t len_parent = strlen(parent);
      size_t k = len_parent - 1;
      for (; k >= 0; --k) if (parent[k] == '/') break;
      copy(&real_parent, parent, k);
      parcours(real_parent, 0, *left_box, 0, &w_main);
    } else {
      if (b > 0) {
        if (strlen(parent) == 0) {
          if (parent != NULL) {
            free(parent);
            parent = NULL;
          }
          parent = malloc(2 * sizeof *parent);
          if (parent == NULL) {
            PRINT("malloc");
          }
          memcpy(parent, "/", 1);
          parent[1] = '\0';
        }
        parcours(parent, 0, *left_box, 0, &w_main);
      }
    }

    if ((*left_box)->n_elements != 0 && r_parent != NULL) {
      for (*previous_pos = 0; *previous_pos < (*left_box)->n_elements; ++*previous_pos) {
        if (!strcmp(r_parent, (*left_box)->menu[*previous_pos].complete_path)) {
          break;
        }
      }
    }

#if defined(EBUG)
    //print_route_positions(&w3, fd, &left_box, parent, pos);
#endif // EBUG

    int position_changed = 0;
    if (*previous_pos >= (*left_box)->n_elements) {
      *previous_pos -= 2;
      if (*previous_pos < 0) { *previous_pos += 2; }
      position_changed = 1;
      //if (*pos < 0) { *pos = 0; }
    }
    if (file_pasted_signal) {
      print_path(s, parent, *pos, 1);
      if (w == &w1) {
        *pos = attributes->pos[0]->m_position;
      }
    } else {
      if (*pos < 0) { *pos = 0;  }
      if (*previous_pos < 0) { *previous_pos = 0; }
      if (w == &w1) {
        print_path(s, (*left_box)->menu[*previous_pos].complete_path, *pos, 1);
      }
    }
    if (position_changed) {
      *previous_pos += 2;
      position_changed = 0;
    }

    //file_pasted_signal = 0;


    if (deleted_file != NULL) {
      free(deleted_file);
      deleted_file = NULL;
    }
    if (real_parent != NULL) {
      free(real_parent);
      real_parent = NULL;
    }

    free(r_parent);
    r_parent = NULL;
    free(parent);
    parent = NULL;
  }
  return 1;
}

int read_tar(Array *left_box, int *pos)
{
  int bytes_read = 4096;
  char buffer[bytes_read];
  pid_t pid;
  int t_read = show_tar(&pid, buffer, &bytes_read, left_box->menu[*pos].complete_path);

  int size_n = 20;
  int *array_n = malloc(size_n * sizeof *array_n);
  if (array_n == NULL) {
    fprintf(stderr, "Error malloc array_n.\n");
    exit(1);
  }

  //int len_buffer = strlen(buffer) + 1;
  int len_buffer = strlen(buffer);
  char *buffer_copy = malloc((len_buffer + 1) * sizeof *buffer_copy);
  if (buffer_copy == NULL) {
    PRINT("Error buffer_copy.");
  }
  memcpy(buffer_copy, buffer, len_buffer);
  buffer_copy[len_buffer] = '\0';

  // ***************************************************************

  int len_argv = strlen(left_box->menu[*pos].complete_path);

  size_t i;
  for (i = len_argv - 1; i >= 0; --i) if (left_box->menu[*pos].complete_path[i] == '/') break;

  char *ext = ".tar.gz";
  int pos_ext = strpos(left_box->menu[*pos].complete_path, ext, 0);

  int remove_ext = (pos_ext - len_argv) * (-1);
  int alloc_n = len_argv - (i + 1) - remove_ext;

  // ***************************************************************

  int x = 0;
  int pos_of_n = 0;
  void *tmp = NULL;
  for (x = 0; x < t_read; ++x) {
    if (buffer[x] == '\n') {
      if (x >= size_n) {
        tmp = realloc(array_n, (size_n *= 2) * sizeof *array_n);
        if (tmp == NULL) {
          PRINT("Error realloc array_n.\n");
        }
        array_n = tmp;
      }
      array_n[pos_of_n++] = x;
    }
  }

  sprintf(position, place_, w2.y_beg + 1, w2.x_beg + 1);
  move(1, position);

  //fsync(STDOUT_FILENO);
  if (pos_of_n > w2.y_size) {
    pos_of_n = w2.y_size;
  }

  int size_alloc = 0;
  char *buf_tmp = NULL;
  for (x = 0; x < pos_of_n; ++x) {
    if (x != 0) {
      size_alloc = array_n[x] - 1 - array_n[x - 1] - alloc_n - 1 ;
      //buf_tmp = malloc(size_alloc * sizeof *buf_tmp);
      buf_tmp = malloc((size_alloc + 1) * sizeof *buf_tmp);
      if (buf_tmp == NULL) {
        PRINT("malloc");
      }
      //strncpy(buf_tmp, &buffer[array_n[x - 1] + 1 + alloc_n + 1], array_n[x] - 1 - array_n[x - 1] - alloc_n - 1);
      memcpy(buf_tmp, &buffer[array_n[x - 1] + 1 + alloc_n + 1], array_n[x] - 1 - array_n[x - 1] - alloc_n - 1);
      buf_tmp[size_alloc ] = '\0';
      print_logos(buf_tmp, buf_tmp);

      write(1, &buffer[array_n[x - 1] + 1 + alloc_n + 1], array_n[x] - 1 - array_n[x - 1] - alloc_n - 1);
    }
    sprintf(position, place_, x + w2.y_beg + 1, w2.x_beg + 1);
    move(1, position);
    if (buf_tmp != NULL) {
      free(buf_tmp);
      buf_tmp = NULL;
    }
  }

  if (array_n != NULL) {
    free(array_n);
    array_n = NULL;
  }

  if (buffer_copy != NULL) {
    free(buffer_copy);
    buffer_copy = NULL;
  }
  return 1;
}

int copy_file3(Array **left_box, int pos)
{
  char *path_to_copied = NULL;
  size_t len_current_folder = 0;
  char *parent = NULL;
  size_t len_folder = strlen((*left_box)->menu[pos].complete_path);
  if (len_folder > 0) {
    size_t i = len_folder - 1;
    for (; i >= 0; --i) {
      if ((*left_box)->menu[pos].complete_path[i] == '/') {
        break;
      }
    }
    if (!(parent = malloc((i + 2) * sizeof *parent))) {
      PRINT("malloc");
    }
    memcpy(parent, (*left_box)->menu[pos].complete_path, i + 1);
    parent[i + 1] = '\0';
  }

  if (parent == NULL) {
    fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
    fprintf(stdout, "left_box.menu[pos].parent is NULL\n");
  }
  if (file_to_be_copied != NULL) {
    size_t slash_pos = (size_t)get_last_slash_pos(file_to_be_copied);
    size_t len_copy = strlen(file_to_be_copied) - slash_pos - 1;
    len_current_folder = strlen(parent);
    if (len_current_folder == 0) {
      fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
      fprintf(stdout, "len_current_folder == 0\n");
    }
    size_t total_copy = len_current_folder + len_copy;
    path_to_copied = malloc((total_copy + 1) * sizeof *path_to_copied);
    if (path_to_copied == NULL) {
      PRINT("malloc");
    }
    memcpy(path_to_copied, parent, len_current_folder);
    memcpy(&path_to_copied[len_current_folder], &file_to_be_copied[slash_pos + 1], len_copy);
    path_to_copied[total_copy] ='\0';
    if (cp(file_to_be_copied, path_to_copied) != 0) {
      return 0;
    }
#if defined(EBUG)
//    write(1, path_to_copied, strlen(path_to_copied));
//    write(1, "\n", strlen("\n"));
#endif // EBUG
    if (path_to_copied != NULL) {
      free(path_to_copied);
      path_to_copied = NULL;
    }

    if ((*left_box)->n_elements != 0) {
      free_array(*left_box);
      (*left_box)->n_elements = 0;
      initialize_array2(&left_box, 1);
    }
    parcours(parent, 0, *left_box, 0, &w_main);
    if (parent != NULL) {
      free(parent);
      parent = NULL;
    }

  }
  return 1;
}

void print_path(Scroll *s, char *path, int pos, int backspace_pressed)
{
  char space[1] = " ";
  int i;
  int len_path = strlen(path);
  int n_slashes = 0;
  int n_size = 2;
  int *pos_slashes = NULL;
  MALLOC(pos_slashes, n_size);

  void *tmp = NULL;
  for (i = 0; i < len_path; ++i) {
    if (path[i] == '/') {
      if (n_slashes >= n_size) {
        tmp = realloc(pos_slashes, (n_size *= 2) * sizeof *pos_slashes);
        if (tmp == NULL) {
          fprintf(stderr, "Error pos_slashes realloc.\n");
          exit(1);
        }
        pos_slashes = tmp;
      }
      pos_slashes[n_slashes++] = i;
    }
  }
  sprintf(position, place_, w1.y_beg - 2, w_main.x_beg + 1);
  move(1, position);
  write(1 , "\033[2K", sizeof("\033[2K"));
  sprintf(position, place_, w1.y_beg - 2, w_main.x_beg + 1);
  move(1, position);
  int j;
  for (j = 1; j < n_slashes; ++j) {
    if (pos_slashes[j ] == pos_slashes[n_slashes - 1] && backspace_pressed == 1) {
      write(1, fg_blue, sizeof(fg_blue));
    }
    write(1, &path[pos_slashes[j - 1] + 1], pos_slashes[j] - pos_slashes[j - 1] - 1);
    write(1, space, 1);
    write(1, fg_reset, sizeof(fg_reset));
    write(1, r_boomerang, strlen(r_boomerang));
    write(1, space, 1);
  }
  write(1, fg_reset, sizeof(fg_reset));
  if (backspace_pressed == 0) {
    write(1,fg_blue, sizeof(fg_blue));
  }
  write(1, &path[pos_slashes[j - 1] + 1], len_path - pos_slashes[j - 1] - 1);
  write(1, space, 1);
  write(1, fg_reset, sizeof(fg_reset));
  sprintf(position, place_, pos - s->pos_upper_t + w1.y_beg + 1, w1.x_beg + 1);
  move(1, position);
  if (pos_slashes != NULL) {
    free(pos_slashes);
    pos_slashes = NULL;
  }
}

int strpos(char *hay, char *needle, int offset)
{
  int len_hay = strlen(hay);
  char haystack[len_hay];

  if (len_hay > 0) {
    strncpy(haystack, hay + offset, len_hay - offset);
    char *ptr = strstr(haystack, needle);
    if (ptr) {
      return ptr - haystack + offset;
    }
  }
  return -1;
}

int show_tar(pid_t *pid, char *buffer, int *bytes_read, char *tar_name)
{
  int total_read = 0;
  int pfd[2];
  pipe(pfd);
  int capacity = *bytes_read;
  if ((*pid = fork()) == 0) {
    close(pfd[0]);                      // child doesn't need read pipe
    dup2(pfd[1], STDOUT_FILENO);        // insure write pipe is at stdout (fd#1)
    dup2(pfd[1], STDERR_FILENO);        // stderr goes to the pipe also (optional)
    close(pfd[1]);                      // child doesn't need to write pipe any more
    execl("/bin/tar", "tar", "-tf", tar_name, (char *)NULL);
    _exit(1);
  } else {
    close(pfd[1]);                      // parent doesn't need write pipe
    while ((*bytes_read = read(pfd[0], &buffer[total_read], capacity - total_read)) > 0)
      total_read += *bytes_read;
    if (total_read > 0)
      buffer[total_read] = '\0';
    wait(NULL);
  }
  return total_read;
}

int match_extension(char *name, const char *ext)
{
  size_t namelen = strlen(name), extlen = strlen(ext);
  return namelen >= extlen && !strcmp(name + namelen - extlen, ext);
}

//#if defined(EBUG)
void handler(int sig)
{
  void *array[10];
  size_t size = backtrace(array, 10);
  restore_config;
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}
//#endif // EBUG

void print_all_attributes(Attributes *attr, int *y_pts_2)
{
  for (int i = 0; i < attr->n_elements; ++i) {
    TTYSTRFD(_file_descriptor_2, *y_pts_2, 1, attr->paths[i]); ++*y_pts_2;
    TTYINTFD(_file_descriptor_2, *y_pts_2, 1, attr->pos[i]->m_position); ++*y_pts_2;
    TTYINTFD(_file_descriptor_2, *y_pts_2, 1, attr->pos[i]->m_lower_pos); ++*y_pts_2;
    TTYINTFD(_file_descriptor_2, *y_pts_2, 1, attr->pos[i]->m_upper_pos); ++*y_pts_2;
  }
}

void print_attributes(Attributes *attr, int *y_pts_2)
{
  for (int i = 0; i < attr->n_elements; ++i) {
    TTYSTRFD(_file_descriptor_2, *y_pts_2, 1, attr->paths[i]); ++*y_pts_2;
  }
}

void reprint_menu(Window_ *w, Scroll *s, Array *a, Attributes *attr, int pos, int option)
{
  if (s->option_previous != option || resized || reprint && pos >= 0 && a->n_elements > 0) {
    if (attr->n_elements != 0) {
#if defined(PRINT_OTHERTTY_2)
      y_pts_2 = 1;
     // erase_scr(_file_descriptor_2, "\033[2J");
      print_attributes(attr, &y_pts_2);
#endif // PRINT_OTHERTTY_2
      int k = attr->n_elements - 1;
      for (; k >= 0; --k) {
        if (!strcmp(a->menu[pos].complete_path, attr->paths[k])) {
          s->pos_upper_t = attr->pos[k]->m_upper_pos;
          s->pos_lower_t = attr->pos[k]->m_lower_pos;
          s->n_lower_t = s->array_size - s->pos_lower_t - 1;
          pos = attr->pos[k]->m_position;
#if defined(PRINT_OTHERTTY_2)
          //erase_scr(_file_descriptor_2, "\033[2J");
          if (w == &w0) {
            TTYINTFD(_file_descriptor_2, y_pts_2, 1, pos); ++y_pts_2;
            TTYINTFD(_file_descriptor_2, y_pts_2, 1, s->pos_upper_t); ++y_pts_2;
            TTYINTFD(_file_descriptor_2, y_pts_2, 1, s->pos_lower_t); ++y_pts_2;
            TTYINTFD(_file_descriptor_2, y_pts_2, 1, s->n_lower_t); ++y_pts_2;
            TTYINTFD(_file_descriptor_2, y_pts_2, 1, s->array_size); ++y_pts_2;
            ++n_passes_for_print_tty_2;
            TTYINTFD(_file_descriptor_2, y_pts_2, 1, n_passes_for_print_tty_2); ++y_pts_2;
            TTYSTRFD(_file_descriptor_2, y_pts_2, 1, "a->menu[pos].complete_path "); ++y_pts_2;
            TTYSTRFD(_file_descriptor_2, y_pts_2, 1, a->menu[pos].complete_path); ++y_pts_2;
          }
#endif // PRINT_OTHERTTY_2
          break;
        }
      }
      option = update(w, s, &pos, a->n_elements);
    }
    if (pos >= 0 && a->n_elements > 0) {
      int i;
      for (i = s->pos_upper_t; i <= s->pos_lower_t; ++i) {
        mv(i - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
        empty_space_debug_fd(1, w->x_size - 2);
        mv(i - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
        if (i == pos) {
          highlight4(w, a, &pos);
        } else if (i < a->n_elements) {
          print(w, a, i);
        }
      }
    }
    // print permissions
    //print_permissions(a, s1, w, pos);
  }
}

void copy_scroll(Scroll *s_in, Scroll *s_out)
{
  s_out->n_to_print = s_in->n_to_print;
  s_out->n_lower_t = s_in->n_lower_t;
  s_out->array_size = s_in->array_size;
  s_out->option_previous = s_in->option_previous;
  s_out->pos_lower_t = s_in->pos_lower_t;
  s_out->pos_upper_t = s_in->pos_upper_t;
}

void indicators(Window_ *w, int y, int x, char pos_c[], char in[], char *msg)
{
  sprintf(pos_c, place_, y, x);
  move(1, pos_c);
  del_from_cursor(in);
  //mvwprintw(w, y, x, msg);
}

void erase_window(Window_ *w, Scroll *s)
{
  int i;
  sprintf(del_in, del, w->x_size - 2);
  for (i = 0; i <  w->y_size - 1; ++i) {
  //for (i = 0; i < n_elements_to_erase; ++i) {
    sprintf(position, place_, i + w->y_beg + 1, w->x_beg + 1);
    move(1, position);
    del_from_cursor(del_in);
  }
}

int print_logos(char *name, char *type)
{
  int horiz_highlight = 0;
  char space[2] = "  ";
  //if (!strcmp(type, "directory"))
  if (*type == 'd')
    //write(1, folder_round_closed_c, strlen(folder_round_closed_c));
  //write(1, folder_clear_closed, strlen(folder_clear_closed));
    write(1, folder_full_closed, strlen(folder_full_closed));
  else if (match_extension(name, "c"))
    write(1, c_file, strlen(c_file));
  else if (match_extension(name, "cpp"))
    write(1, c_plus_plus, strlen(c_plus_plus));
  else if (match_extension(name, "sh"))
    write(1, terminal2, strlen(terminal2));
  else if (match_extension(name, "png") ||
      match_extension(name, "jpg") ||
      match_extension(name, "jpeg"))
    write(1, image3, strlen(image3));
  else if (match_extension(name, "doc") ||
      match_extension(name, "docx") ||
      match_extension(name, "odt"))
    write(1, word_document, strlen(word_document));
  else if (match_extension(name, "xls") ||
      match_extension(name, "xlsx"))
    write(1, word_document, strlen(word_document));
  else if (match_extension(name, "ppt") ||
      match_extension(name, "pptx"))
    write(1, word_document, strlen(word_document));
  else if (match_extension(name, "vimrc"))
    write(1, vim2, strlen(vim2));
  else if (match_extension(name, "pdf"))
    write(1, adobe, strlen(adobe));
  else if (match_extension(name, "md"))
    write(1, mark_down, strlen(mark_down));
  else if (match_extension(name, "gz"))
    write(1, tar, strlen(tar));
  else if (!strcmp(name, "LICENSE"))
    write(1, license_logo, strlen(license_logo));
  else if (match_extension(name, ".gitignore"))
    write(1, git_full, strlen(git_full));
  else if (match_extension(name, "README"))
    write(1, mark_down, strlen(mark_down));
  else if (match_extension(name, "mp4") || match_extension(name, "MP4"))
    write(1, video3, strlen(video3));
  else if (!strcmp(type, "file"))
    write(1, file_logo2, strlen(file_logo2));
  //write(1, file_logo, strlen(file_logo));
  else
    write(1, file_logo2, strlen(file_logo2));
  write(1, space, strlen(space));
  return horiz_highlight;
}

void highlight4(Window_ *w, Array *a, int *pos)
{
  size_t len_space = 1;
  size_t len_logo = 2;
  size_t len_vert_bars = 2;
  if (*pos < a->n_elements) {
    int len_entry = strlen(a->menu[*pos].name);
    int space_available_in_win = w->x_size - len_vert_bars - len_space - len_logo;
    int horiz = 0;
    if (len_entry > space_available_in_win) {
      len_entry = space_available_in_win;
      //horiz = len_entry;
    } else {
      horiz = w->x_size - len_entry - len_vert_bars - len_space - len_logo;
    }

    write_sz(bg_cyan);
    print_logos(a->menu[*pos].name, a->menu[*pos].type);
    write_partial(a->menu[*pos].name, len_entry);

    int i;
    if (horiz > 0) {
      char space[1] = " ";
      for (i = 0; i < horiz; ++i) {
        if (i == horiz - 2 && image_cp_signal && file_to_be_copied != NULL && strcmp(file_to_be_copied, a->menu[*pos].complete_path) == 0) {
          size_t len_copy_logo = strlen(copy_files);
          write_partial(copy_files, len_copy_logo);
        } else {
          write_partial(space, 1);
          //write_len(space);
        }
      }
    }
    //if (write(1, bg_reset, sizeof(bg_reset)) < 0) { exit(1); }
    write_sz(bg_reset);
  }
}

static void sig_win_ch_handler(int sig) { resized = 1; }

int update(Window_ *w, Scroll *s, int *pos, int size)
{
  int option = 0;
//  if (image_used == 0) {
    //sem_wait(&mutex);
    s->array_size = size;

    int y = w->y_size - 1;
    if (*pos == 0 ) {
      s->pos_upper_t = 0;
    }
    if (s->array_size <= y) {
      s->n_to_print = s->array_size;
      s->pos_lower_t = s->n_to_print - 1;
      s->n_lower_t = 0;
      if (s->option_previous == 3) {
        s->pos_upper_t = 0;
      }
      if (s->array_size == y) {
        option = 1;
      } else if (s->array_size < y) {
        option = 2;
      }
    } else if (s->array_size > y) {
      s->n_to_print = y ;
      if (s->n_to_print < s->pos_upper_t || s->pos_upper_t != 0) {
        s->pos_lower_t = s->n_to_print + s->pos_upper_t - 1;
      } else {
        s->pos_lower_t = s->n_to_print - s->pos_upper_t - 1;
      }
      if (s->pos_lower_t < 0) { s->pos_lower_t = s->array_size - 1; }
      s->n_lower_t = s->array_size - s->pos_lower_t - 1;

      if (s->n_to_print + s->pos_upper_t >= s->array_size) {
        s->n_to_print = s->array_size - s->pos_upper_t;
        s->pos_lower_t = s->array_size - 1;
        s->n_lower_t = s->array_size - s->pos_lower_t - 1;
      }
      if (s->n_to_print != w->y_size - 1) {
        s->n_to_print = w->y_size  - 1;
        if (s->n_to_print > s->array_size - 1) {
          s->n_to_print = s->array_size - 1;
        }
        s->pos_upper_t = s->pos_lower_t - s->n_to_print;
        s->pos_lower_t = s->pos_upper_t + s->n_to_print - 1;
        s->n_lower_t = s->array_size - s->pos_lower_t - 1;
      }
      if (*pos == s->array_size - 1 && resized) {
        *pos = s->pos_lower_t;
        if (s->pos_lower_t == s->array_size - 1) {
          *pos = s->array_size - 1;
          if (debug) {
#if defined(EBUG)
            indicators(&w_main, w_main.y_size - 2, w_main.x_beg + 2, position, del_in, "lower == size");
#endif // EBUG
          }
        }
        if (s->pos_lower_t != s->array_size - 1) {
          ++s->pos_upper_t;
          ++s->pos_lower_t;
          ++*pos;
        }
      }
      option = 3;
    }
    s->option_previous = option;
    //sem_post(&mutex);
  //}

  return option;
}

void print(Window_ *w, Array *a, int pos_array)
{
  size_t len_space = 1;
  size_t len_logo = 2;
  size_t len_vert_bars = 2;
  int len = strlen(a->menu[pos_array].name);
  if (len > w->x_size - 2 - len_space - len_logo) {
    len = w->x_size - 2 - len_space - len_logo;
  }
  print_logos(a->menu[pos_array].name, a->menu[pos_array].type);
  if (image_cp_signal && image_cp_pos == pos_array) {
    write_partial(a->menu[pos_array].name, len);
    for (int i = 0; i < w->x_size - len - len_vert_bars - len_space - len_logo - 2; ++i) {
      write_len(" ");
    }
    size_t len_copy_logo = strlen(copy_files);
    write_partial(copy_files, len_copy_logo);
  } else {
    write_partial(a->menu[pos_array].name, len);
  }
}

void move_erase(Window_ *w, int fd, int y, int x)
{
  sprintf(del_in, del, w->x_size - 2);
  sprintf(position, place_, y, x);
  move(fd, position);
  del_from_cursor(del_in);
  sprintf(position, place_, y, x);
  move(fd, position);
}

void print_entries(Window_ *w, Scroll *s, __attribute__((__unused__)) char **entries,
                   int option, int c, int *pos, Array *a)
{
  int i;
  int y = 0;
  char in[strlen(del) + 27];
  sprintf(del_in, del, w->x_size - 2);
  previous_position_before_backspace = *pos;

#if defined(EBUG)
  Message msg = { 0 };
  msg.print_msg = "pos print_entries = ";
  msg.n_ulong = *pos;
  msg.used_ulong = 1;
  print_message2(w, s, 6, *pos, &msg);
#endif // EBUG

  switch (c) {
    case KEY_UP:
    case UP:
      if (*pos > 0) {
        if (image_used == 0) {
          --*pos;
        }
        //resized = 0;
        if (*pos >= s->pos_upper_t && *pos < s->pos_lower_t) {
          y = *pos - s->pos_upper_t + w->y_beg + 1;

          move_erase(w, 1, y + 1, w->x_beg + 1);

          print(w, a, *pos + 1);

          sprintf(position, place_, y, w->x_beg + 1);
          move(1, position);
        } else if (*pos <= s->pos_upper_t && s->pos_upper_t > 0) {
          --s->pos_upper_t;
          ++s->n_lower_t;
          --s->pos_lower_t;
          for (i = 0; i < s->n_to_print; ++i) {

            move_erase(w, 1, i + w->y_beg + 1, w->x_beg + 1);
            if (s->pos_upper_t + i < s->array_size + w->y_beg) {
              print(w, a, s->pos_upper_t + i);
            }
          }
          //del_from_cursor(del_in);
          sprintf(position, place_, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
          move(1, position);
        }
        if (*pos >= s->pos_lower_t) {
          *pos = s->pos_lower_t;
          if (*pos < s->array_size - 1) {

            move_erase(w, 1, *pos + w->y_beg - s->pos_upper_t + 1, w->x_beg + 1);
          }
        }

        if (*pos < s->pos_lower_t + 1) {
          //sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
          //del_from_cursor(del_in);
          highlight4(w, a, pos);
        }
      }
      break;
    case KEY_DOWN:
    case DN:
      if (*pos < s->array_size - 1) {
        //if (image_used == 0) {
        //  ++*pos;
        //}
        ++*pos;

/*
        if (*pos - 1 >= s->pos_lower_t) {
          --s->pos_upper_t;
          ++s->n_lower_t;
          --s->pos_lower_t;
        }
*/
        //resized = 0;
        if (*pos > s->pos_upper_t && *pos <= s->pos_lower_t) {
          y = *pos - s->pos_upper_t + w->y_beg;
          move_erase(w, 1, y, w->x_beg + 1);
          if (*pos - 1 < s->array_size) {
            print(w, a, *pos - 1);
          }
          sprintf(position, place_, y + 1, w->x_beg + 1);
          move(1, position);
          outside_box = 0;
        } else if (*pos - 1 >= s->pos_lower_t) {
          ++s->pos_upper_t;
          --s->n_lower_t;
          ++s->pos_lower_t;

          for (i = 0; i < s->n_to_print; ++i) {
            move_erase(w, 1, i + w->y_beg + 1, w->x_beg + 1);
            if (s->pos_upper_t + i < s->array_size) {
              print(w, a, s->pos_upper_t + i);
            }
          }

          if (*pos - s->pos_upper_t + w->y_beg + 1 < w->y_size - w->y_beg + 1) {
            outside_box = 0;
            move_erase(w, 1, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
          } else {
            outside_box = 1;
            sprintf(position, place_, i + w->y_beg, w->x_beg + 1);
            move(1, position);
          }
        }
        if (outside_box) {
          *pos = s->pos_lower_t;
        }
        highlight4(w, a, pos);
      }
      break;
    case KEY_PAGE_UP:
      //char pg_up[] = "pg_up";

      //sprintf(in, del, w->x_size - 2);
      if (*pos - s->n_to_print + 1 >= 0) {

        if (*pos - s->n_to_print + 1 >= s->pos_upper_t) {

          if (debug) {
#if defined(EBUG)
            sprintf(position, place_, w_main.y_size - 1, w->x_beg + 2);
            move(1, position);
            write(1, pg_up, strlen(pg_up));
            indicators(w, w_main.y_size - 2, w->x_beg + 1, position, del_in, "1aup");
#endif // EBUG
          }

          if (*pos - s->pos_upper_t - w->y_beg + 1 < w->y_size - 1) { //
            if (s->n_to_print > 1) {

              move_erase(w, 1, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);

              print(w, a, *pos);
            }
            *pos -= (s->n_to_print - 1); // donne *pos = *pos - s->n_to_print + 1
            sprintf(position, place_, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
            move(1, position);

          } else {
            sprintf(position, place_, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
            move(1, position);
            del_from_cursor(del_in);
            *pos = s->pos_lower_t;
            sprintf(position, place_, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
            move(1, position);
          }
          highlight4(w, a, pos);

        } else {
          if (debug) {
#if defined(EBUG)
            indicators(w, w_main.y_size - 2, w->x_beg + 1, position, del_in, "1bup");
#endif // EBUG
          }

          move_erase(w, 1, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
          print(w, a, *pos);

          if (*pos - s->n_to_print + 1 < s->pos_upper_t) {
            *pos -= (s->n_to_print - 1);
            s->pos_upper_t = *pos;
            s->pos_lower_t = s->pos_upper_t + s->n_to_print - 1;
            s->n_lower_t = s->array_size - s->pos_lower_t - 1;

          }
          if (*pos > s->pos_lower_t) {
            *pos = s->pos_lower_t;
          }
          for (i = 0; i < s->n_to_print; ++i) {

            move_erase(w, 1, i + w->y_beg + 1, w->x_beg + 1);

            if (*pos + i < s->array_size - 1) {
              print(w, a, *pos + i);
            }
          }

          sprintf(position, place_, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
          move(1, position);
          highlight4(w, a, pos);
        }
      } else if (*pos - s->n_to_print < 0) {
        if (debug) {
#if defined(EBUG)
          indicators(w, w_main.y_size - 2, w->x_beg + 1, position, del_in, "2up");
#endif // EBUG
        }

        *pos = 0;

        //sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
        //move(1, position);
        for (i = 0; i < s->n_to_print; ++i) {

          sprintf(position, place_, i + w->y_beg + 1, w->x_beg + 1);
          move(1, position);
          del_from_cursor(del_in);

          if (*pos + i < s->array_size) {
            print(w, a, *pos + i);
          }
        }

        s->pos_lower_t -= s->pos_upper_t;
        s->pos_upper_t = *pos;
        s->n_lower_t = s->array_size - s->n_to_print;
        sprintf(position, place_, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
        move(1, position);

        highlight4(w, a, pos);
      }
      break;
    case KEY_PAGE_DN:
      //char pg_dn[] = "pg_dn";

      //PRINTVALUE(c);
      sprintf(in, del, w->x_size - 2);
      if (*pos + s->n_to_print - 1 <= s->array_size - 1) {
        if (debug) {
//#if defined(EBUG)
          sprintf(position, place_, w_main.y_size - 2, w->x_beg + 2);
          move(1, position);
          write(1, pg_dn, strlen(pg_dn));
          indicators(w, w_main.y_size - 1, w->x_beg + 2, position, del_in, "1");
//#endif // EBUG
        }

        if (*pos + s->n_to_print - 1 <= s->pos_lower_t) { // no scroll

          move_erase(w, 1, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
          print(w, a, *pos);
          *pos += (s->n_to_print - 1);
        } else {
          if (debug) {
//#if defined(EBUG)
            indicators(w, w->y_size + 3, w->x_beg + 1, position, del_in, "1a");
//#endif // EBUG
          }
          s->pos_upper_t = *pos;
          s->pos_lower_t = *pos + s->n_to_print - 1;
          s->n_lower_t = s->array_size - s->pos_lower_t - 1;
          if (s->n_lower_t < 0) {
            s->n_lower_t = 0;
          }
          // va ds 2 ne s'applique pas ici
          //if (s->pos_lower_t > s->array_size - 1) {
          //    s->pos_lower_t = s->array_size - 1;
          //}
          for (i = 0; i < s->n_to_print; ++i) {
            move_erase(w, 1, i + w->y_beg + 1, w->x_beg + 1);
            if (*pos + i < s->array_size) {
              print(w, a, *pos + i);
              //write(1, "a", strlen("a"));
            }
          }
          *pos += (s->n_to_print - 1);
        }
        sprintf(position, place_, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
        move(1, position);
        highlight4(w, a, pos);

      } else if (*pos + s->n_to_print - 1 > s->array_size - 1) {
        if (debug) {
#if defined(EBUG)
          indicators(w, w_main.y_size - 2, w->x_beg + 1, position, del_in, "2  ");
#endif // EBUG
        }

        for (i = 0; i < s->n_to_print; ++i) {
          move_erase(w, 1, i + w->y_beg + 1, w->x_beg + 1);
          print(w, a, s->pos_upper_t + s->n_lower_t + i);
        }

        s->pos_upper_t += s->n_lower_t;
        s->pos_lower_t = s->array_size - 1;
        s->n_lower_t = 0;

        *pos = s->array_size - 1;

        sprintf(position, place_, s->n_to_print + w->y_beg, w->x_beg + 1);
        move(1, position);
        highlight4(w, a, pos);
      } else if (*pos + 1 < s->array_size - 1) {
        del_from_cursor(in);
        mvwprintw(w, a, w->y_size + 3, w->x_beg + 2, "3", *pos + 1);
      }
      break;
    case KEY_HOME:
    case KEY_ALL_UP:
      *pos = 0;
// and backspace used
// replaces the highlighted elements at where they were before entering the folder
// seems that x11 can't 'ungrab' key home and key end keys
      s->pos_upper_t = 0;
      s->pos_lower_t = s->n_to_print - 1;
      s->n_lower_t = s->array_size - s->n_to_print;
      for (i = 0; i < s->n_to_print; ++i) {
        sprintf(position, place_, i + w->y_beg + 1, w->x_beg + 1);
        move(1, position);
        del_from_cursor(del_in);
        print(w, a, i);
      }
      sprintf(position, place_, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
      move(1, position);
      highlight4(w, a, pos);
      break;
    case KEY_END:
      s->pos_upper_t = s->array_size - s->n_to_print;
      *pos = s->array_size - 1;
// replaces the highlighted elements at where they were before entering the folder
// seems that x11 can't 'ungrab' key home and key end keys
      s->pos_lower_t = *pos;
      s->n_lower_t = 0;
      for (i = 0; i < s->n_to_print; ++i) {
        sprintf(position, place_, i + w->y_beg + 1, w->x_beg + 1);
        move(1, position);
        del_from_cursor(del_in);
        print(w, a, i + s->pos_upper_t);
      }
      sprintf(position, place_, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
      move(1, position);
      highlight4(w, a, pos);
      break;
    case 4:
        //scroll_window3(&w1, a, s, pos);
        //scroll_window4(&w1, a, s, pos);
        scroll_window5(&w1, a, s, pos);
      break;
    case 21:
      scroll_window_up7(&w1, a, s, pos);
      break;
    default:
      break;
  }

  if (*pos == 0) {
    sprintf(position, place_, w->y_beg + 1, w->x_beg + 1);
    move(1, position);
  }

  snprintf(position, strlen(place_), place_, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
  move(1, position);
}

int show_all_85()
{
  //space
  int len = 10;
  background_blue
  string_normal
  space_str
  foreground_blue
  background_cyan
  right_full_triangle
  background_cyan
  foreground_cyan
  background_reset
  right_full_triangle
  foreground_blue
  right_line_triangle
  //foreground_reset
  return len;
}

int show_all_855(int y, int x)
{
  //space
  int len = 10;
  background_blue
  mv(y, x);
  string_normal
  space_str
  foreground_blue
  background_cyan
  right_full_triangle
  background_cyan
  foreground_cyan
  background_reset
  background_blue
  right_full_triangle
  foreground_blue
  //right_line_triangle
  //mv(y, x + 11);
  //background_blue
  background_blue
  //background_blue
  //foreground_reset
  return len;
}

int show_all_8555(int y, int x)
{
  int len = 5;
  background_blue
  mv(y, x);
  string_mode_n
  space_str
  foreground_blue
  background_cyan
  right_full_triangle
  background_cyan
  foreground_cyan
  background_reset
  background_blue
  right_full_triangle
  foreground_blue
  background_blue
  return len;
}

void show_status_line(Window_ *w, Array *a, Scroll *s, int pos)
{
  // lower status line
  int vert = w->y_size;
  mv(vert + w->y_beg + 1, w->x_beg);
  if (number_of_windows == 2 && w == &w1 || number_of_windows == 3 && w == &w0) {
    int len_airline = show_all_8555(vert + w->y_beg + 1, w->x_beg);
    int j;
    int len_permissions = strlen(a->menu[pos].permissions);
    for (j = 0; j < w_main.x_size - len_airline - len_permissions - 1; ++j) {
      space_str
      if (j == 1) {
        foreground_reset
        print_permissions(a, s, w, pos);
        foreground_blue
        background_blue
      }
    }
    foreground_reset
    background_reset
  }
  mv(pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
}

void draw_box(Window_ *w)
{
  int i = 1, j = 0;
  int horiz = w->x_size;
  int vert = w->y_size;
  BOX_CONTOUR(line, v_line,
              lu_corner, ll_corner, ru_corner, rl_corner,
              heavy_line, heavy_v_line,
              lu_heavy_corner, ll_heavy_corner, ru_heavy_corner, rl_heavy_corner,
              heavy_uppert_corner, heavy_v_up, heavy_lowert_corner);
  if (box_color && box_thickness) {
    write_sz(fg_cyan);
  }
  // upper left corner
  mv(w->y_beg, w->x_beg);
  if (w != &w2) {
    write_len(ARRAY[cont_2]);
  } else if (w == &w2) {
    write_len(ARRAY[cont_6]);
  }
  // upper horizontal line
  for (; j < horiz - 1; ++j) {
    write_len(ARRAY[cont_0]);
  }
  // upper right corner
  if (w == &w2) {
    mv(w->y_beg, w->x_beg + horiz);
    write_len(ARRAY[cont_4]);
  }
  // both vertical lines
  for (; i < vert ; ++i) {
    mv(w->y_beg + i  , w->x_beg);
    if (w != &w2) {
      write_len(ARRAY[cont_1]);
    } else {
      write_len(ARRAY[cont_1]);
      mv(w->y_beg + i , w->x_beg + horiz);
      write_len(ARRAY[cont_1]);
    }
  }
  // lower left corner
  mv(vert + w->y_beg, w->x_beg);
  if (w == &w2) {
    write_len(ARRAY[cont_8]);
  } else {
    write_len(ARRAY[cont_3]);
  }
  // lower horizontal line
  mv(vert + w->y_beg, w->x_beg + 1);
  for (j = 0; j < horiz - 1; ++j) {
    write_len(ARRAY[cont_0]);
  }
  // lower right corner
  if (w == &w2) {
    mv(vert + w->y_beg, w->x_beg + horiz);
    write_len(ARRAY[cont_5]);
  }
  write_sz(fg_reset);
/*
  // lower status line
  mv(vert + w->y_beg + 1, w->x_beg);
  //int len_airline = show_all_85();
  if (number_of_windows == 2 && w == &w1 || number_of_windows == 3 && w == &w0) {
    int len_airline = show_all_855(vert + w->y_beg + 1, w->x_beg);
    for (j = 0; j < w_main.x_size - len_airline - 2; ++j) {
      space_str
    }
    foreground_reset
    background_reset
  }
*/
  mv(vert / 2, w->x_beg + 1);
}

//#define len_menu "len = %d"
void mvwprintw(Window_ *win, Array *a, int y, int x, char *str, int pos)
{
  if (win->y_size > w_main.y_size) {
    char *err = "Error: out of window size: ";
    write(2, err, strlen(err));
    write(2, __func__, strlen(__func__));
  }

  //int len = strlen(str);
  int len = strlen(a->menu[pos].name);
  sprintf(position, place_, w_main.y_beg + 1, w_main.x_beg + 1);
  move(1, position);
  del_from_cursor(del_in);
  /*
     char l_menu[sizeof(len_menu)];
     sprintf(l_menu, len_menu, len);
     write(1, l_menu, strlen(l_menu));
     */
  size_t len_space = 1;
  size_t len_logo = 2;

  if (len > win->x_size - 2 - len_space - len_logo) {
    len = win->x_size - 2 - len_space - len_logo;
  }
  sprintf(position, place_, y, x);
  move(1, position);
  del_from_cursor(del_in);
  move(1, position);

  //write(1, l_menu, strlen(l_menu));
  print_logos(a->menu[pos].name, a->menu[pos].type);
  write(1, str, len);
}

#define snprint   "s->n_to_print: %d"
#define sposupper "s->pos_upper_t: %d"

#define snlower   "s->n_lower_t: %d"

#define len_num   "horiz: %d"

#define Y_SIZE    "y_size: %d"
#define Y_BEG     "y_beg : %d"
#define X_SIZE    "x_size: %d"
#define X_BEG     "x_beg : %d"
#define BACK_PRESSED "back_pressed: %d"

#define ATTR_N "attr_number: %d"

#define ATTRIBUTES_PATH "attr[size-1]: %s"

#define PREVIOUS_POS "previous_pos : %d"
#define PREVIOUS_POS_FROM_ATTR "previous_pos_from_attr : %d"

void print_debug(Window_ *w, Scroll *s, int option, int pos, int cursor_pos, Array *a)
{
/*
  char back[sizeof(BACK_PRESSED)];
  sprintf(position, place, 1, w->x_beg + 25);
  move(1, position);
  del_from_cursor(del_in);
  sprintf(back, BACK_PRESSED, back_pressed);
  write(1, back, strlen(back));
*/
  char x_size[sizeof("%d")];
  char y_size[sizeof("%d")];
  //char pos_place_[sizeof(place_) + 1];
  char pos_place_[sizeof(place_) + 27];

  sprintf(pos_place_, place_, 1, w->x_size - 3);
  move(1, pos_place_);
  sprintf(y_size, "%d", w->y_size);
  write(1, y_size, strlen(y_size));

  sprintf(pos_place_, place_, 2, w->x_size - 3);
  move(1, pos_place_);
  sprintf(x_size, "%d", w->x_size);
  write(1, x_size, strlen(x_size));

  sprintf(position, place_, w->y_size - 3, (w->x_size / 2) - 30);
  move(1, position);
  char ny_size[sizeof(Y_SIZE)];
  sprintf(ny_size, Y_SIZE, w1.y_size);
  write(1, ny_size, strlen(ny_size));

  sprintf(position, place_, w->y_size - 2, (w->x_size / 2) - 30);
  move(1, position);
  char nx_size[sizeof(X_SIZE)];
  sprintf(nx_size, X_SIZE, w1.x_size);
  write(1, "\033[2K", sizeof("\033[2K"));
  write(1, nx_size, sizeof(nx_size));

  char num[strlen(snprint)];

  char attributes_num[sizeof(ATTR_N)];
  sprintf(attributes_num, ATTR_N, attributes->n_elements);
  sprintf(position, place_, w->y_size - 4, w->x_beg + 2);
  move(1, position);
  del_from_cursor(del_in);
  write(1, attributes_num, sizeof(attributes_num));

  sprintf(pos_place_, place_, w->y_beg + 1, w->x_beg + 1);
  move(1, pos_place_);
  del_from_cursor(del_in);
  sprintf(num, snprint, s->n_to_print);
  write(1, num, strlen(num));

  sprintf(position, place_, w->y_size - 3, (w->x_size / 2) - 10);
  move(1, position);
  int len = strlen(a->menu[pos].name);
  int horiz = 0;
  //char num_len[sizeof(len_num) + 1]; // a cause du signe negatif ajouter + 1
  if (len > w1.x_size - w1.x_beg - 4) {
    len = w1.x_size - w1.x_beg - 4;
    horiz = w1.x_size - len;
  }
  if (horiz < 0) {
    horiz *= -1;
  }

  char num_pos[strlen(sposupper)];

  sprintf(pos_place_, place_, w->y_beg + 1, (w->x_size / 2) + 5);
  move(1, pos_place_);
  sprintf(num_pos, sposupper, s->pos_upper_t);
  del_from_cursor(del_in);
  sprintf(pos_place_, place_, w->y_beg + 1, (w->x_size / 2) + 5);
  move(1, pos_place_);
  write(1, num_pos, strlen(num_pos));

  sprintf(pos_place_, place_, w->y_size - 2, (w->x_size / 2) + 5);
  move(1, pos_place_);

  sprintf(pos_place_, place_, w->y_beg + 1, (w->x_size / 2) + 5);
  move(1, pos_place_);
  sprintf(num_pos, sposupper, s->pos_upper_t);
  del_from_cursor(del_in);
  sprintf(pos_place_, place_, w->y_beg + 1, (w->x_size / 2) + 5);
  move(1, pos_place_);
  write(1, num_pos, strlen(num_pos));

  sprintf(pos_place_, place_, w->y_size - 2, (w->x_size / 2) + 5);
  move(1, pos_place_);


  del_from_cursor(del_in);
  sprintf(pos_place_, place_, w->y_size - 2, (w->x_size / 2) - 1);
  move(1, pos_place_);
  sprintf(num_pos, "s->pos_lower_t: %d", s->pos_lower_t);
  write(1, num_pos, strlen(num_pos));

  char num_lower[strlen(snlower)];
  sprintf(pos_place_, place_, w->y_size - 2, (w->x_size / 2) + 19);
  move(1, pos_place_);
  del_from_cursor(del_in);
  sprintf(num_lower, snlower, s->n_lower_t);
  write(1, num_lower, strlen(num_lower));

  char opt[10];
  sprintf(opt, "option: %d", option);
  sprintf(pos_place_, place_, w->y_size - 3, (w->x_size / 2) + 10);
  move(1, pos_place_);
  write(1, opt, strlen(opt));

  sprintf(pos_place_, place_, w->y_size - 4, (w->x_size / 2) + 10);
  move(1, pos_place_);
  del_from_cursor(del_in);
  sprintf(pos_place_, place_, w->y_size - 4, (w->x_size / 2) + 10);
  move(1, pos_place_);
  sprintf(opt, "pos:    %d", pos);
  sprintf(pos_place_, place_, w->y_size - 4, (w->x_size / 2) + 10);
  move(1, pos_place_);
  write(1, opt, strlen(opt));

  //char pos_c[strlen(place_)];
  char pos_c[sizeof(place_) + 27];
  sprintf(pos_c, place_, w->y_size + 4, w->x_size - 20);
  move(1, pos_c);
  char de[sizeof(del)];
  sprintf(de, del, 20);

  //char attr_first[sizeof("attr[size-1]: %s")];
  //sprintf(pos_c, place, w->y_size + 2, w->x_size - 60);
  //move(1, pos_c);
  //sprintf(de, del, 30);
  //del_from_cursor(de);
  //sprintf(pos_c, place, w->y_size + 2, w->x_size - 60);
  //move(1, pos_c);
  /*
     if (attributes.n_elements != 0) {
     sprintf(attr_first, ATTRIBUTES_PATH, attributes.paths[attributes.n_elements - 1]);
     write(1, attr_first, strlen(attr_first));
     }
     */
  char attr_arr[sizeof("attr_arr: %s")];
  sprintf(pos_c, place_, w->y_size - 1, w->x_beg + 1);
  move(1, pos_c);
  sprintf(de, del, 40);
  del_from_cursor(de);
  sprintf(pos_c, place_, w->y_size - 1, w->x_beg + 1);
  move(1, pos_c);
  sprintf(attr_arr, "attr_arr: %s", attributes->paths[0]);
  write(1, attr_arr, strlen(attr_arr));

  char entr_bckspc[sizeof("enter_backspace: %d")];
  sprintf(pos_c, place_, w->y_size, w->x_beg);
  move(1, pos_c);
  sprintf(de, del, 20);
  del_from_cursor(de);
  sprintf(pos_c, place_, w->y_size, w->x_beg);
  move(1, pos_c);
  sprintf(entr_bckspc, "enter_backspace: %d", enter_backspace);
  write(1, entr_bckspc, strlen(entr_bckspc));



  char prev_pos[strlen(PREVIOUS_POS)];
  char prev_pos_from_attr[strlen(PREVIOUS_POS_FROM_ATTR)];
  //del_from_cursor(del_in);
  //sprintf(pos_place, place, w->y_size - 2, (int)(w->x_size + strlen(entr_bckspc) + 1));
  //move(1, pos_place);

  //sprintf(pos_c, place, w->y_size, (int)(w->x_beg + strlen(entr_bckspc) + 5));
  //move(1, pos_c);
  //sprintf(de, del, 20);
  //del_from_cursor(de);
  sprintf(pos_c, place_, w->y_size, (int)(w->x_beg + strlen(entr_bckspc) + 5));
  move(1, pos_c);

  sprintf(prev_pos, PREVIOUS_POS, previous_pos_copy);
  write(1, prev_pos, strlen(prev_pos));


  sprintf(pos_c, place_, w->y_size, (int)(w->x_beg + strlen(entr_bckspc) + strlen(prev_pos) + 10));
  move(1, pos_c);
  sprintf(prev_pos_from_attr, PREVIOUS_POS_FROM_ATTR, previous_pos_copy_from_attr);
  write(1, prev_pos_from_attr, strlen(prev_pos_from_attr));

  //sprintf(pos_place_, place, w->y_size - 2, (w->x_size) + 5);
  //move(1, pos_place);

  // replace le curseur a la 1ere lettre de la dern entree
  sprintf(pos_c, place_, cursor_pos, w1.x_beg + 1);
  move(1, pos_c);
}

// print in another tty c program
void print_attributes_debug(Window_ *w, Scroll *s, int option, int pos,
                            int cursor_pos, Array *a, Attributes *attributes, int fd)
{
  int len_first = 0;
  char *attr_first_entry = NULL;
  int len_last = 0;
  char *attr_last_entry = NULL;
  sprintf(del_in, "\033[%dX", 40);

  //char byte;

  if (attributes->n_elements != 0) {
    len_first = strlen(attributes->paths[0]);
    attr_first_entry = (char *)malloc(sizeof(char) * (len_first + 1));
    if (attr_first_entry) {
      strncpy(attr_first_entry, attributes->paths[0], len_first);
      attr_first_entry[len_first] = '\0';
    }

    sprintf(position, place_, w->y_size - 4, w->x_beg);
    move(1, position);
    del_from_cursor(del_in);
    write(1, attr_first_entry, strlen(attr_first_entry));


    struct winsize w_s;
    //Window win_pts;
    //win_pts.y_beg = 1;
    //win_pts.x_beg = 1;
    if (!ioctl(fd, TIOCGWINSZ, &w_s)) {
      write(fd, attr_first_entry, strlen(attr_first_entry));
    }


    if (attributes->n_elements > 1) {
      len_last = strlen(attributes->paths[attributes->n_elements - 1]);
      attr_last_entry = (char *)malloc(sizeof(char) * (len_last + 1));
      if (attr_last_entry) {
        strncpy(attr_last_entry, attributes->paths[attributes->n_elements - 1], len_last);
        attr_last_entry[len_last] = '\0';
      }
      sprintf(position, place_, w->y_size - 3, w->x_beg);
      move(1, position);
      del_from_cursor(del_in);
      write(1, attr_last_entry, strlen(attr_last_entry));
      write(fd, attr_last_entry, strlen(attr_last_entry));
    }
  }

  if (attr_first_entry != NULL)
    free(attr_first_entry);
  if (attr_last_entry != NULL)
    free(attr_last_entry);

  sprintf(position, place_, cursor_pos, w1.x_beg + 1);
  move(1, position);
}
/* vim: foldmethod=marker tabstop=2 shiftwidth=2 expandtab
 * */
