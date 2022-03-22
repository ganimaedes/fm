#include "array.h"
#include "scr.h"
#include <unistd.h>
#define _XOPEN_SOURCE

#include "parcours.h"
#include "deb.h"
#include "copy.h"
#include "img.h"
#include <execinfo.h>
#include <signal.h>
#include <stddef.h>

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
volatile sig_atomic_t image_used = 0;
volatile sig_atomic_t modify_pos_bc_image_used = 0;
volatile sig_atomic_t previous_position_before_backspace = 0;

char position[PLACE_SZ];
char del_in[IN_SZ];

Window_ w_main, w1, w2, w3;
Attributes attributes;
char *file_to_be_copied;

static int previous_pos_copy = 0;
static int previous_pos_copy_from_attr = 0;

/*
typedef struct _MsgBool {
} MsgBool;
*/
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

void print_path(Scroll *s, char *path, int pos, int backspace_pressed);
int strpos(char *hay, char *needle, int offset);
int show_tar(pid_t *pid, char *buffer, int *bytes_read, char *tar_name);
int match_extension(char *name, const char *ext);
void handler(int sig);
void reprint_menu(Window_ *w, Scroll *s1, Array *a, Attributes *attr, int pos, int option);
void copy_scroll(Scroll *s_in, Scroll *s_out);
void indicators(Window_ *w, int y, int x, char pos_c[], char in[], char *msg);
void erase_window(Window_ *w, Scroll *s);
int print_logos(char *name, char *type);
void highlight2(Array *a, int *pos);
int update(Window_ *w, Scroll *s, int *pos, int size);
void print_debug(Window_ *w, Scroll *s, int option, int pos, int cursor_pos, Array *a);
void move_erase(Window_ *w, int fd, int y, int x);
//void print_entries(Window_ *w, Scroll *s, char **entries, int option, unsigned int *c, int *pos, Array *a);
//void print_entries(Window_ *w, Scroll *s, char **entries, int option, unsigned long *c, int *pos, Array *a);
//void print_entries(Window_ *w, Scroll *s, char **entries, int option, unsigned long c, int *pos, Array *a);
void print_entries(Window_ *w, Scroll *s, char **entries, int option, int c, int *pos, Array *a);
void print(Window_ *w, Array *a, int pos_array);
static void sig_win_ch_handler(int sig);
void mvwprintw(Window_ *win, Array *a, int y, int x, char *str, int pos);
void draw_box(Window_ *w);
void print_attributes_debug(Window_ *w, Scroll *s, int option, int pos,
    int cursor_pos, Array *a, Attributes *attributes, int fd);
int copy_file2(Array *left_box, int pos);
int read_tar(Array *left_box, int *pos);
int getBackSpaceFolder(Array *left_box, int *pos, int *previous_pos, Scroll *s);
int directory_placement(Array *left_box, Array *right_box, Scroll *s, int *pos, Window_ *w1, Window_ *w2, Window_ *w_main);
int window_resize(Window_ *w_main,
                  Window_ *w1,
                  Window_ *w2,
                  struct winsize *w_s,
                  Scroll *s,
                  Array *left_box,
                  Array *right_box,
                  int *option,
                  int *pos,
                  int *initial_loop,
                  volatile sig_atomic_t *resized, int *i);
int del_file(Window_ *w1, Scroll *s, Array *left_box, int *pos, int *option);
void print_n_elements(Array *left_box);
void print_permissions(Array *a, Scroll *s1, Window_ *w, int pos);
//void print_message(Window_ *w, Scroll *s, int position_from_end_scr, char *msg, unsigned int number, int *pos);
void print_message(Window_ *w, Scroll *s, int position_from_end_scr, char *msg, unsigned long number, int *pos);
//void print_message2(Window_ *w, Scroll *s, int position_from_end_scr, int *pos, Message *msg);
void print_message2(Window_ *w, Scroll *s, int position_from_end_scr, int pos, Message *msg);

int main(int argc, char **argv)
{
  char **entries = NULL;
#if defined(EBUG)
  signal(SIGSEGV, handler);
#endif // EBUG
  if (argc < 2) { fprintf(stderr, "Missing Arguments\n"); exit(1); }
  setlocale(LC_ALL, "");
  save_config;

  char position[strlen(place_)];
  if (get_window_size(0, 1, &w_main.y_size, &w_main.x_size) < 0) {
    restore_config;
    char *error = "Error getting window size";
    sprintf(position, place_, w1.y_beg + 1, w1.x_beg + 1);
    move(1, position);
    write(1, error, strlen(error));
    return EXIT_FAILURE;
  }

  int pos = 0;

  w_main.y_beg = 1;
  w_main.x_beg = 1;
  w_main.y_previous = 0;
  w_main.x_previous = 0;

  w1.y_beg = 3;
  w1.x_beg = 1;
  w1.y_previous = 0;
  w1.x_previous = 0;

  w2.y_beg = w1.y_beg;
  w2.x_beg = (w_main.x_size / 2);

  Scroll s;
  s.option_previous = 0;

  struct winsize w_s;
  struct sigaction sact;
  sigemptyset(&sact.sa_mask);
  sact.sa_flags = 0;
  sact.sa_handler = sig_win_ch_handler;
  char err1[] = "sigaction";
  if (sigaction(SIGWINCH, &sact, NULL)) {
    restore_config;
    write(2, err1, sizeof(err1));
  }

  unsigned long c = 0;
  int previous_pos_c = 0;
  int second_previous_c = 0;
  int option = 0,
      i,
      initial_loop = 1,
      secondary_loop = 0,
      previous_pos = pos,
      left_allocation = 1;
  int backspace = 0;

  Array left_box;
  Array right_box;
  init(&left_box, 5);
  init(&right_box, 5);

  Positions posit;
  posit.m_lower_pos = 0;
  posit.m_position = 0;
  posit.m_upper_pos = 0;

  attributes.paths = NULL;
  attributes.pos = NULL;
  attributes.counter = 0;
  attributes.n_elements = 0;
  attributes.capacity = 0;
  init_attr(&attributes, 2);

  parcours(argv[1], 0, &left_box, 0, &w_main);

  int fd = 0;

  int position_before_copying = 0;

  int yank_counter = 0;
  int delete_counter = 0;

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


  //int image_used = 0;

  info_key_presses.keypress_value = 0;
  info_key_presses.n_times_pressed = 0;

  for (;;) {
    if (!ioctl(0, TIOCGWINSZ, &w_s)) {
      window_resize(&w_main, &w1, &w2, &w_s, &s, &left_box, &right_box, &option, &pos, &initial_loop, &resized, &i);
    }

    //if (image_used) {
      //c = second_previous_c;
      //c = 1;
    //}
    //image_used = 0;
    //char *pos_str = "pos = ";

#if defined(EBUG)
    msg.print_msg = "previous_pos = ";
    msg.n_ulong = previous_pos;
    msg.used_ulong = 1;
    print_message2(&w_main, &s, 1, pos, &msg);
#endif // EBUG
    if (left_box.n_elements != 0) {
      //if (previous_pos < left_box.n_elements) {
        if (backspace) {
          pos = previous_pos;
          backspace = 0;
        }
      //}

      if (!left_allocation || secondary_loop) {
        reprint = 1;
        secondary_loop = 0;
      }

      if (c == KEY_ENTER || c == KEY_BACKSPACE || resized || c == ENTER || c == BACKSPACE) {

        previous_pos_copy = previous_pos;
        if (attributes.n_elements > 0) {
          previous_pos_copy_from_attr = attributes.pos[attributes.n_elements - 1]->m_position;

          if (file_pasted_signal) {
            position_before_copying = pos;
            position_before_copying_sig = 1;
            pos = previous_pos_copy_from_attr;
            reprint_menu(&w1, &s, &left_box, &attributes, pos, option);
            file_pasted_signal = 0;
          } else {
            reprint_menu(&w1, &s, &left_box, &attributes, pos, option);
          }
        } else {
          reprint_menu(&w1, &s, &left_box, &attributes, pos, option);
        }
      }

      if (c == 'c' || c == 'x' || c == 'y') {
        if (c == 'y') {
          ++yank_counter;
        }
        if (yank_counter != 2) {
          if (file_to_be_copied) {
            free(file_to_be_copied);
            file_to_be_copied = NULL;
          }
          size_t len_copy = strlen(left_box.menu[pos].complete_path);
          copy(&file_to_be_copied, left_box.menu[pos].complete_path, len_copy);
          if (c == 'x') {
            file_to_be_moved_signal = 1;
          }
          yank_counter = 0;
        }
      } else if (c == 'p') {
        int result_copy = copy_file2(&left_box, pos);
        if (file_to_be_moved_signal) {
          unlink(file_to_be_copied); // unlink
          file_to_be_moved_signal = 0;
        }
        if (result_copy && file_to_be_copied) {
          free(file_to_be_copied);
          file_to_be_copied = NULL;
        }

        n_elements_to_erase = left_box.n_elements;

        erase_window(&w1, &s);
        reprint_menu(&w1, &s, &left_box, &attributes, pos, option);
        file_pasted_signal = 1;
      } else if (c == KEY_SUPPR || c == 'd') {
        if (c == 'd') {
          ++delete_counter;
        }
        if (delete_counter != 2) {
          del_file(&w1, &s, &left_box, &pos, &option); //attributes missing in IN/OUT
          delete_counter = 0;
        }
      }
      //char *MSG = "c = ";
      //if (c > 400) { c = 107; }
      //print_message(&w_main, &s, 4, MSG, c, &pos);
      // mettre une autre struct enregistrant les keypresses

      //if (c > 400) { c = 107; pos = 0; sleep(5); }
      //if (c == -17) { c = 107; }

      //if (image_used) {
      //  pos = previous_pos;
      //}
      print_entries(&w1, &s, entries, option, (int)c, &pos, &left_box);
      image_used = 0;
      print_permissions(&left_box, &s, &w1, pos);

#if defined(EBUG)
      msg.n_ulong = c;
      msg.used_ulong = 1;
      msg.print_msg = "keypress = ";
      print_message2(&w1, &s, 0, pos, &msg);
      //msg.used_ulong = 0;
#endif // EBUG

/*
      msg.print_msg = "pos = ";
      msg.n_ulong = pos;
      msg.used_ulong = 1;
      print_message2(&w1, &s, 6, pos, &msg);
      if (modify_pos_bc_image_used && back_pressed) {
        pos = previous_position_before_backspace;
        modify_pos_bc_image_used = 0;
      }

*/

      if (pos < left_box.n_elements && !strcmp(left_box.menu[pos].type, "directory")) {
        directory_placement(&left_box, &right_box, &s, &pos, &w1, &w2, &w_main);

      } else if (pos < left_box.n_elements &&
                 (match_extension(left_box.menu[pos].name, "gz") ||
                 match_extension(left_box.menu[pos].name, "xz"))) {
        read_tar(&left_box, &pos);
        sprintf(position, place_, pos - s.pos_upper_t + w1.y_beg + 1, w1.x_beg + 1);
        move(1, position);
      } else if (match_extension(left_box.menu[pos].name, "jpeg") ||
                 match_extension(left_box.menu[pos].name, "jpg") ||
                 match_extension(left_box.menu[pos].name, "png")) {
        //while (!kbhit());
        // soit kbhit is trying to get char at the same time as XGet or XGrabKey error
//./min -id 0x<WINDOW_ID> <IMAGE_PATH> 0.5 980 50
        //set_img(6, "fm", 0x200008, left_box.menu[pos].complete_path, 0.5, 50, 980);
        //set_img(6, "fm", 0x200008, left_box.menu[pos].complete_path, 0.5, w2.x_beg + w1.x_size, w2.y_beg);
        //set_img(6, "fm", 0x200006, left_box.menu[pos].complete_path, 0.5, w2.y_px_size, w1.x_px_size);
        //c = set_img(6, "fm", 0x200006, left_box.menu[pos].complete_path, 1, w2.y_px_size, w1.x_px_size);
        //c = set_img(0, NULL, 0, left_box.menu[pos].complete_path, 1, w2.y_px_size, w1.x_px_size);
        //ai to see deleted pics
        //draw_box for when passing from two windows to three windows
        //c = set_img(0, NULL, 0, left_box.menu[pos].complete_path, 1, 0, 0);
        c = set_img(left_box.menu[pos].complete_path, &info_key_presses);
        if (info_key_presses.n_times_pressed > 1) {
          msg.print_msg = "n_times_pressed = ";
          msg.n_ulong = info_key_presses.n_times_pressed;
          msg.used_ulong = 1;
          print_message2(&w_main, &s, 1, pos, &msg);
          info_key_presses.n_times_pressed = 0;
          n_times_keypressed = 0;
          n_times_keypressed_copy = 0;
          //sleep(5);
        }
        if (c == KEY_ALL_UP) {
          //sleep(10);
        }
        //if (c == XKEY)
/*
        if (c == KEY_END) {
          ungetc(c, stdin);
        }
*/
/*
        snprintf(position, strlen(place_), place_, w_main.y_size - 4, w_main.x_beg + 1);
        move(1, position);
#define value_return "c = %d"
        char val_return[sizeof(value_return)];
        sprintf(val_return, value_return, c);
        write(1, val_return, strlen(val_return));
        snprintf(position, strlen(place_), place_, pos - s.pos_upper_t + w1.y_beg + 1, w1.x_beg + 1);
        move(1, position);
*/
        image_used = 1;
        //if (c == KEY_BACKSPACE) {
        //  enter_backspace = 1;
        //  back_pressed = 1;
        //}
        //if (c >= 262) { c = 107; }
        //printf("y size in px = %u, x size in px = %u\n", w2.y_px_size, w2.x_px_size);
        //set_img(6, "fm", 0x200008, left_box.menu[pos].complete_path, 0.5, , );
      }
    }

    if (enter_backspace == 1 && attributes.n_elements != 0 && back_pressed == 1 && initial_loop != 1) {
      free_attr(&attributes);
      init_attr(&attributes, 1);
    }

#if defined(EBUG)
      int p = pos - s.pos_upper_t + w1.y_beg + 1;
      print_debug(&w_main, &s, option, pos, p, &left_box);
      if (argv[2] != NULL && initial_loop == 1) {
        fd = open(argv[2], O_RDWR);
        save_config_fd(fd);
      }
      //print_tty(&w3, fd, &attributes);
#endif // EBUG


    if (image_used == 0 && (c = kbget()) == KEY_ESCAPE) {
      break;
    } else if ((c == 'l' || c == KEY_ENTER || c == ENTER) &&
               !strcmp(left_box.menu[pos].type, "directory") &&
               right_box.n_elements != 0) {
      if (position_before_copying_sig) {
        position_before_copying_sig = 0;
        pos = position_before_copying;
      }

      n_elements_to_erase = left_box.n_elements;

      previous_pos = pos;
      erase_window(&w1, &s);
      ++enter_backspace;

      posit.m_position = pos;
      posit.m_upper_pos = s.pos_upper_t;
      posit.m_lower_pos = s.pos_lower_t;
      add_attr(&attributes, &posit, left_box.menu[pos].complete_path);

      print_path(&s, left_box.menu[previous_pos].complete_path, pos, 0);

      if (left_box.n_elements != 0) {
        free_array(&left_box);
        left_box.n_elements = 0;
        init(&left_box, right_box.n_elements);
      }
      dupArray2(&right_box, &left_box);

      n_elements_to_erase = right_box.n_elements;

      erase_window(&w2, &s);
      if (right_box.n_elements != 0) {
        free_array(&right_box);
        right_box.n_elements = 0;
        init(&right_box, 1);
      }
      pos = 0;
      option = update(&w1, &s, &pos, left_box.n_elements);
      secondary_loop = 1;
      back_pressed = 0;


#if defined(EBUG)
      print_n_elements(&left_box);
#endif // EBUG


    } else if (c == 'h' || c == KEY_BACKSPACE || c == BACKSPACE) {

      n_elements_to_erase = left_box.n_elements;

      second_previous_c = previous_pos_c;

      erase_window(&w1, &s);
      --enter_backspace;

      if (left_box.n_elements != 0 && strlen(left_box.menu[pos].complete_path) != 1) {
        // chercher le parent avec l'inode place en ordre decroissant
        getBackSpaceFolder(&left_box, &pos, &previous_pos, &s);
      }

      //n_elements_to_erase = right_box.n_elements;

      erase_window(&w2, &s);

      if (right_box.n_elements != 0) {
        free_array(&right_box);
        right_box.n_elements = 0;
        if (left_box.n_elements != 0) {
          init(&right_box, left_box.n_elements);
        }
      }

      option = update(&w1, &s, &pos, left_box.n_elements);
      left_allocation = 0;
      secondary_loop = 1;
      backspace = 1;
      back_pressed = 1;

#if defined(EBUG)
      print_n_elements(&left_box);
#endif // EBUG

    }
    previous_pos_c = c;

    erase_window(&w2, &s);
    initial_loop = 0;
    resized = 0;
    if (image_used) {
      modify_pos_bc_image_used = 1;
      image_used = 0;
    }
  }
  if (left_box.n_elements != 0 || left_box.capacity != 0) {
    free_array(&left_box);
  }
  if (right_box.n_elements != 0 || right_box.capacity != 0) {
    free_array(&right_box);
  }
  if (attributes.n_elements != 0 || attributes.capacity != 0) {
    free_attr(&attributes);
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
  restore_config;
  return 0;
}

void init_msg_struct(Message *msg)
{

}

void print_message(Window_ *w, Scroll *s, int position_from_end_scr, char *msg, unsigned long number, int *pos)
{
  snprintf(position, strlen(place_), place_, w->y_size - position_from_end_scr, w->x_beg + 1);
  move(1, position);
#define value_return "c = %lu"
  //CAT(msg);
  //write(1, CAT(msg), strlen(CAT(msg)));
  //write(1, " = ", strlen(" = "));
  char val_return[sizeof(value_return)];
  sprintf(val_return, value_return, number);
  write(1, val_return, strlen(val_return));
  snprintf(position, strlen(place_), place_, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
  move(1, position);
}

//void print_in_char(char *defined_value, ...)
void print_in_char(char *msg, ...)
{
  va_list args = { 0 };
  va_start(args, msg);
  vfprintf(stderr, msg, args);
  //write(1, val_return, strlen(val_return));
  va_end(args);
  //char val_return[sizeof(defined_value)];
  //sprintf(val_return, value_return, number);
  //write(1, val_return, strlen(val_return));
}

#define CONCAT(a, b) CONCAT(a, b)
#define CONCAT_(a, b) a ## b


/*
 * Concatenate preprocessor tokens A and B without expanding macro definitions
 * (however, if invoked from a macro, macro arguments are expanded).
 */
#define PPCAT_NX(A, B) A ## B

/*
 * Concatenate preprocessor tokens A and B after macro-expanding them.
 */
#define PPCAT(A, B) PPCAT_NX(A, B)
/*
 * Turn A into a string literal without expanding macro definitions
 * (however, if invoked from a macro, macro arguments are expanded).
 */
#define STRINGIZE_NX(A) #A

/*
 * Turn A into a string literal after macro-expanding it.
 */
#define STRINGIZE(A) STRINGIZE_NX(A)


//void print_message2(Window_ *w, Scroll *s, int position_from_end_scr, int *pos, Message *msg)
void print_message2(Window_ *w, Scroll *s, int position_from_end_scr, int pos, Message *msg)
{
  snprintf(position, strlen(place_), place_, w->y_size - position_from_end_scr, w->x_beg + 1);
  move(1, position);
  del_from_cursor(del_in);
  if (msg->used_ulong) {
#ifndef value_return
#define value_return " %lu"

#endif
  //char value_str[sizeof(CONCAT(msg->print_msg, value_return))];
  //char *nth = CONCAT(msg->print_msg, value_return);
  //sprintf(, value_str, msg->print_msg);
  //char value_str[sizeof(STRINGIZE(PPCAT_NX(msg->print_msg, value_return)))];
  //char *nth = STRINGIZE(PPCAT_NX(msg->print_msg, value_return));
  //sprintf(nth, value_str, msg->n_ulong);
  //print_in_char(value_return, msg->n_ulong);
  //print_in_char(value_return, CAT(msg->n_ulong));
  //print_in_char(nth);
  //char *nth = STRINGIZE_NX(value_return);
  //char *nth = "%s ";
  char *nth = STRINGIZE(msg->print_msg);
  char *mth = STRINGIZE_NX(value_return);
  //print_in_char(STRINGIZE(PPCAT_NX(msg->print_msg, STRINGIZE_NX(value_return))), msg->n_ulong);
  //print_in_char(STRINGIZE_NX(msg->print_msg), STRINGIZE(PPCAT_NX(nth, STRINGIZE_NX(value_return))), msg->n_ulong);
  write(1, msg->print_msg, strlen(msg->print_msg));
  write(1, " ", strlen(" ")),
  print_in_char(STRINGIZE(PPCAT_NX(nth, STRINGIZE_NX(value_return))), msg->n_ulong);
  //print_in_char(STRINGIZE(PPCAT_NX(msg->print_msg, value_return)), msg->n_ulong);
  //print_in_char(STRINGIZE(PPCAT_NX(msg->print_msg, nth)), msg->n_ulong);
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
  //char val_return[sizeof(value_return)];
  //sprintf(val_return, value_return, number);
  //write(1, val_return, strlen(val_return));
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
  // print permissions
//  if (pos < a->n_elements -  1) {
    sprintf(position, place_, w_main.y_size - 2, w->x_beg + 1);
    move(1, position);
    del_from_cursor(del_in);
    write(1, a->menu[pos].permissions, strlen(a->menu[pos].permissions));
    sprintf(position, place_, pos - s1->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
    move(1, position);
//  }
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

int del_file(Window_ *w1, Scroll *s, Array *left_box, int *pos, int *option)
{
  int supp_word = 0;
  supp_word = kbget();
  if (supp_word == 'y' || supp_word == 'Y') {
    write(1, "DELETE ", strlen("DELETE "));
  }
  size_t _i = strlen(left_box->menu[*pos].complete_path) - 1;
  for (; _i >= 0; --_i) if (left_box->menu[*pos].complete_path[_i] == '/') break;
  char *_parent = NULL;
  _parent = malloc((_i + 1) * sizeof *_parent);
  if (_parent == NULL) {
    PRINT("malloc");
  }
  memcpy(_parent, left_box->menu[*pos].complete_path, _i);
  _parent[_i] = '\0';

  //unlink(left_box->menu[*pos].complete_path);

  if (left_box->n_elements != 0) {
    free_array(left_box);
    left_box->n_elements = 0;
    init(left_box, 1);
  }
  erase_window(w1, s);
  parcours(_parent, 0, left_box, 0, &w_main);
  if (left_box->n_elements > 0 && *pos >= left_box->n_elements - 1) {
    *pos = left_box->n_elements - 1;
  }
  reprint_menu(w1, s, left_box, &attributes, *pos, *option);
  if (_parent) {
    free(_parent);
    _parent = NULL;
  }
  return 1;
}


int window_resize(Window_ *w_main,
                  Window_ *w1,
                  Window_ *w2,
                  struct winsize *w_s,
                  Scroll *s,
                  Array *left_box,
                  Array *right_box,
                  int *option,
                  int *pos,
                  int *initial_loop,
                  volatile sig_atomic_t *resized, int *i)
{
  w_main->y_size = w_s->ws_row;
  w_main->x_size = w_s->ws_col;

  w1->y_size = w_main->y_size - w1->y_beg - 5;
  w1->x_size = w_main->x_size / 2;

///*
  w2->y_beg = w1->y_beg;
  w2->x_beg = w1->x_size;
  w2->y_size = w1->y_size;
  w2->x_size = w1->x_size ;
//*/

  w2->y_px_size = w_s->ws_ypixel;
  w2->x_px_size = w_s->ws_xpixel;
/*
  w2->y_beg = w1->y_beg;
  //w2->y_beg = w1->y_beg + (w_main->y_size / 2);
  w2->x_beg = w1->x_size + (w1->x_size / 2) + 1;
  w2->y_size = w1->y_size;
  w2->x_size = w1->x_size ;
*/
  sprintf(del_in, del, w1->x_size - 2);

  if (w1->y_size > w1->y_previous || w1->x_size > w1->x_previous ||
      w1->y_size < w1->y_previous || w1->x_size < w1->x_previous) {
    erase_scr(1, "\033[2J");

    draw_box(w1);
    draw_box(w2);
    *option = update(w1, s, pos, left_box->n_elements);

    if (*initial_loop) {
      sprintf(position, place_, w1->y_beg + 1, w1->x_beg + 1);
      move(1, position);
      highlight2(left_box, pos);
      for (*i = 1; *i < s->n_to_print; ++*i) {
        mvwprintw(w1, left_box, *i + w1->y_beg + 1, w1->x_beg + 1, left_box->menu[*i].name, *i);
      }
    }
    *resized = 1;
  }
  w1->y_previous = w1->y_size;
  w1->x_previous = w1->x_size;
  return 1;
}

int directory_placement(Array *left_box, Array *right_box, Scroll *s, int *pos, Window_ *w1, Window_ *w2, Window_ *w_main)
{
  if (right_box->n_elements != 0) {
    free_array(right_box);
    init(right_box, 1);
  }
  parcours(left_box->menu[*pos].complete_path, 0, right_box, 0, w_main);
  int to_print = 0;
  if (right_box->n_elements <= s->n_to_print) {
    to_print = right_box->n_elements;
  } else {
    if (right_box->n_elements >= w2->y_size - 1) {
      to_print = w2->y_size - 1;
    } else {
      to_print = right_box->n_elements;
    }
  }
  size_t i;
  for (i = 0; i < to_print; ++i) {
    mvwprintw(w2, right_box, i + w2->y_beg + 1, w2->x_beg + 1, right_box->menu[i].name, i);
  }
  sprintf(position, place_, *pos - s->pos_upper_t + w1->y_beg + 1, w1->x_beg + 1);
  move(1, position);
  return 1;
}

int getBackSpaceFolder(Array *left_box, int *pos, int *previous_pos, Scroll *s)
{
  char *parent = NULL;
  size_t length_parent = strlen(left_box->menu[*pos].complete_path);
  size_t p = length_parent - 1, counter = 0;
  for (; p >= 0; --p) if (left_box->menu[*pos].complete_path[p] == '/' && ++counter == 2) break;
  if (p < length_parent - 1 && length_parent != 1) {
    if (length_parent != 1) {
      for (; p >= 0; --p) if (left_box->menu[*pos].complete_path[p] == '/') break;
    }
    copy(&parent, left_box->menu[*pos].complete_path, p);
    char *r_parent = NULL;
    int b = strlen(left_box->menu[*pos].complete_path) - 1;
    for (; b >= 0; --b) { if (left_box->menu[*pos].complete_path[b] == '/') { break; } }
    copy(&r_parent, left_box->menu[*pos].complete_path, b);
    free_array(left_box);
    left_box->n_elements = 0;
    init(left_box, 1);

    char *real_parent = NULL;
    if (file_pasted_signal) {
      size_t len_parent = strlen(parent);
      size_t k = len_parent - 1;
      for (; k >= 0; --k) if (parent[k] == '/') break;
      copy(&real_parent, parent, k);
      parcours(real_parent, 0, left_box, 0, &w_main);
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
        parcours(parent, 0, left_box, 0, &w_main);
      }
    }

    if (left_box->n_elements != 0 && r_parent != NULL) {
      for (*previous_pos = 0; *previous_pos < left_box->n_elements; ++*previous_pos) {
        if (!strcmp(r_parent, left_box->menu[*previous_pos].complete_path)) {
          break;
        }
      }
    }

#if defined(EBUG)
    //print_route_positions(&w3, fd, &left_box, parent, pos);
#endif // EBUG

    int position_changed = 0;
    if (*previous_pos >= left_box->n_elements) {
      *previous_pos -= 2;
      if (*previous_pos < 0) { *previous_pos += 2; }
      position_changed = 1;
      //if (*pos < 0) { *pos = 0; }
    }
    if (file_pasted_signal) {
      print_path(s, parent, *pos, 1);
      *pos = attributes.pos[0]->m_position;
    } else {
      if (*pos < 0) { *pos = 0;  }
      if (*previous_pos < 0) { *previous_pos = 0; }
      print_path(s, left_box->menu[*previous_pos].complete_path, *pos, 1);
    }
    if (position_changed) {
      *previous_pos += 2;
      position_changed = 0;
    }

    //file_pasted_signal = 0;

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

  int len_buffer = strlen(buffer) + 1;
  char *buffer_copy = malloc(len_buffer * sizeof *buffer_copy);
  if (buffer_copy == NULL) {
    fprintf(stderr, "Error buffer_copy.\n");
    exit(1);
  }
  strncpy(buffer_copy, buffer, len_buffer);
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
          fprintf(stderr, "Error realloc array_n.\n");
          exit(1);
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
      buf_tmp = malloc(size_alloc * sizeof *buf_tmp);
      strncpy(buf_tmp, &buffer[array_n[x - 1] + 1 + alloc_n + 1], array_n[x] - 1 - array_n[x - 1] - alloc_n - 1);
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

int mv_to_trash(Array *left_box, int pos)
{
  char *path_to_copied = NULL;
  size_t len_current_folder = 0;
  char *parent = NULL;
  size_t len_folder = strlen(left_box->menu[pos].complete_path);
  if (len_folder > 0) {
    size_t i = len_folder - 1;
    for (; i >= 0; --i) {
      if (left_box->menu[pos].complete_path[i] == '/') {
        break;
      }
    }
    if (!(parent = malloc((i + 2) * sizeof *parent))) {
      PRINT("malloc");
    }
    memcpy(parent, left_box->menu[pos].complete_path, i + 1);
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

    if (left_box->n_elements != 0) {
      free_array(left_box);
      left_box->n_elements = 0;
      init(left_box, 1);
    }
    parcours(parent, 0, left_box, 0, &w_main);
    if (parent != NULL) {
      free(parent);
      parent = NULL;
    }

  }
  return 1;
}

int copy_file2(Array *left_box, int pos)
{
  char *path_to_copied = NULL;
  size_t len_current_folder = 0;
  char *parent = NULL;
  size_t len_folder = strlen(left_box->menu[pos].complete_path);
  if (len_folder > 0) {
    size_t i = len_folder - 1;
    for (; i >= 0; --i) {
      if (left_box->menu[pos].complete_path[i] == '/') {
        break;
      }
    }
    if (!(parent = malloc((i + 2) * sizeof *parent))) {
      PRINT("malloc");
    }
    memcpy(parent, left_box->menu[pos].complete_path, i + 1);
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

    if (left_box->n_elements != 0) {
      free_array(left_box);
      left_box->n_elements = 0;
      init(left_box, 1);
    }
    parcours(parent, 0, left_box, 0, &w_main);
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
  int *pos_slashes = malloc(n_size * sizeof *pos_slashes);
  if (pos_slashes == NULL) {
    fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
    fprintf(stdout, "malloc\n");
    exit(1);
  }
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
  strncpy(haystack, hay + offset, len_hay - offset);
  char *ptr = strstr(haystack, needle);
  if (ptr)
    return ptr - haystack + offset;
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

#if defined(EBUG)
void handler(int sig)
{
  void *array[10];
  size_t size = backtrace(array, 10);
  restore_config;
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}
#endif // EBUG

void reprint_menu(Window_ *w, Scroll *s1, Array *a, Attributes *attr, int pos, int option)
{
  if (s1->option_previous != option || resized || reprint) {

    if (attr->n_elements != 0) {
      int k = attr->n_elements - 1;
      for (; k >= 0; --k) {
        if (!strcmp(a->menu[pos].complete_path, attr->paths[k])) {
          s1->pos_upper_t = attr->pos[k]->m_upper_pos;
          s1->pos_lower_t = attr->pos[k]->m_lower_pos;
          s1->n_lower_t = s1->array_size - s1->pos_lower_t - 1;
          pos = attr->pos[k]->m_position;
          break;
        }
      }
      option = update(w, s1, &pos, a->n_elements);
    }
    int i;
    for (i = s1->pos_upper_t; i <= s1->pos_lower_t; ++i) {
      sprintf(position, place_, i - s1->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
      move(1, position);
      if (i == pos) {
        del_from_cursor(del_in);
        sprintf(position, place_, pos - s1->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
        move(1, position);
        highlight2(a, &pos);
      } else if (i < a->n_elements) {
        print(w, a, i);
      }
    }
    // print permissions
    print_permissions(a, s1, w, pos);
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
  if (!strcmp(type, "directory"))
    write(1, folder_round_closed_c, strlen(folder_round_closed_c));
  //write(1, folder_clear_closed, strlen(folder_clear_closed));
  //write(1, folder_full_closed, strlen(folder_full_closed));
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

#define len_menu "len = %d"
void highlight2(Array *a, int *pos)
{
  size_t len_space = 1;
  size_t len_logo = 2;
  int len = strlen(a->menu[*pos].name);
  if (len > w1.x_size - 2 - len_space - len_logo)
    len = w1.x_size - 2 - len_space - len_logo;

  // ************************************************

  /*
     sprintf(position, place, w_main.y_beg + 1, w_main.x_beg + 1);
     move(1, position);
     del_from_cursor(del_in);
     char l_menu[sizeof(len_menu)];
     sprintf(l_menu, len_menu, len);
     write(1, l_menu, strlen(l_menu));

     sprintf(position, place, *pos + w1.y_beg + 1, w1.x_beg + 1);
     move(1, position);
     */

  // ************************************************

  write(1, bg_cyan, sizeof(bg_cyan));
  print_logos(a->menu[*pos].name, a->menu[*pos].type);
  int horiz = w1.x_size - len - 2 - len_space - len_logo;
  write(1, a->menu[*pos].name, len);

  int i;
  if (horiz > 0) {
    char space[1] = " ";
    for (i = 0; i < horiz; ++i)
      write(1, space, 1);
  }
  write(1, bg_reset, sizeof(bg_reset));
}

static void sig_win_ch_handler(int sig) { resized = 1; }

int update(Window_ *w, Scroll *s, int *pos, int size)
{
  int option = 0;
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

  return option;
}

void print(Window_ *w, Array *a, int pos_array)
{
  size_t len_space = 1;
  size_t len_logo = 2;
  int len = strlen(a->menu[pos_array].name);
  if (len > w->x_size - 2 - len_space - len_logo) {
    len = w->x_size - 2 - len_space - len_logo;
  }
  print_logos(a->menu[pos_array].name, a->menu[pos_array].type);
  write(1, a->menu[pos_array].name, len);
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

//void print_entries(Window_ *w, Scroll *s, __attribute__((__unused__)) char **entries,
//                   int option, unsigned int *c, int *pos, Array *a)
//void print_entries(Window_ *w, Scroll *s, __attribute__((__unused__)) char **entries,
//                   int option, unsigned long *c, int *pos, Array *a)
//void print_entries(Window_ *w, Scroll *s, __attribute__((__unused__)) char **entries,
//                   int option, unsigned long c, int *pos, Array *a)
void print_entries(Window_ *w, Scroll *s, __attribute__((__unused__)) char **entries,
                   int option, int c, int *pos, Array *a)
{
  int i;
  int y = 0;
  char in[strlen(del)];
  sprintf(del_in, del, w->x_size - 2);
  previous_position_before_backspace = *pos;

#if defined(EBUG)
  Message msg = {};
  msg.print_msg = "pos print_entries = ";
  msg.n_ulong = *pos;
  msg.used_ulong = 1;
  print_message2(w, s, 6, *pos, &msg);
#endif // EBUG

  switch (c) {
    case KEY_UP:
    case UP:
      if (*pos > 0) {
        --*pos;
/*
      if (modify_pos_bc_image_used) {
        *pos = previous_position_before_backspace;
        modify_pos_bc_image_used = 0;
      }
*/
        resized = 0;
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

      if (modify_pos_bc_image_used) {
        *pos = previous_position_before_backspace;
        modify_pos_bc_image_used = 0;
      }
          if (*pos < s->array_size - 1) {

            move_erase(w, 1, *pos + w->y_beg - s->pos_upper_t + 1, w->x_beg + 1);
          }
        }

        if (*pos < s->pos_lower_t + 1) {
          //sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
          //del_from_cursor(del_in);
          highlight2(a, pos);
        }
      }
      break;
    case KEY_DOWN:
    case DN:
      if (*pos < s->array_size - 1) {
        ++*pos;

/*
      if (modify_pos_bc_image_used) {
        *pos = previous_position_before_backspace;
        modify_pos_bc_image_used = 0;
      }
*/
        resized = 0;
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
        highlight2(a, pos);
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
            //del_from_cursor(in);
            del_from_cursor(del_in);
            *pos = s->pos_lower_t;
            sprintf(position, place_, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
            move(1, position);
          }
          highlight2(a, pos);

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
          highlight2(a, pos);
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

        highlight2(a, pos);
      }
      break;
    case KEY_PAGE_DN:
      //char pg_dn[] = "pg_dn";

      sprintf(in, del, w->x_size - 2);
      if (*pos + s->n_to_print - 1 <= s->array_size - 1) {
        if (debug) {
#if defined(EBUG)
          sprintf(position, place_, w_main.y_size - 2, w->x_beg + 2);
          move(1, position);
          write(1, pg_dn, strlen(pg_dn));
          indicators(w, w_main.y_size - 1, w->x_beg + 2, position, del_in, "1");
#endif // EBUG
        }

        if (*pos + s->n_to_print - 1 <= s->pos_lower_t) { // no scroll

          move_erase(w, 1, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
          print(w, a, *pos);
          *pos += (s->n_to_print - 1);
        } else {
          if (debug) {
#if defined(EBUG)
            indicators(w, w->y_size + 3, w->x_beg + 1, position, del_in, "1a");
#endif // EBUG
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
        highlight2(a, pos);

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
        highlight2(a, pos);
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
/*
      if (modify_pos_bc_image_used && back_pressed) {
        *pos = previous_position_before_backspace;
        modify_pos_bc_image_used = 0;
      }
*/
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
      highlight2(a, pos);
      break;
    case KEY_END:
      s->pos_upper_t = s->array_size - s->n_to_print;
      *pos = s->array_size - 1;
// replaces the highlighted elements at where they were before entering the folder
// seems that x11 can't 'ungrab' key home and key end keys
      if (modify_pos_bc_image_used && back_pressed) {
        *pos = previous_position_before_backspace;
        modify_pos_bc_image_used = 0;
      }
//
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
      highlight2(a, pos);
      break;
    default:
      break;
  }

  //if (image_used) {
  //  *pos = previous_position_before_backspace;
  //}

  if (*pos == 0) {
    sprintf(position, place_, w->y_beg + 1, w->x_beg + 1);
    move(1, position);
  }

  snprintf(position, strlen(place_), place_, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
  move(1, position);
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
    write(1, fg_cyan, sizeof(fg_cyan));
  }
  // upper left corner
  sprintf(position, place_, w->y_beg, w->x_beg);
  move(1, position);
  if (w != &w2) {
    write(1, ARRAY[cont_2], strlen(ARRAY[cont_2]));
  } else if (w == &w2) {
    write(1, ARRAY[cont_6], strlen(ARRAY[cont_6]));
  }
  // upper horizontal line
  for (; j < horiz - 1; ++j) {
    write(1, ARRAY[cont_0], strlen(ARRAY[cont_0]));
  }
  // upper right corner
  if (w == &w2) {
    sprintf(position, place_, w->y_beg, w->x_beg + horiz );
    move(1, position);
    write(1, ARRAY[cont_4], strlen(ARRAY[cont_4]));
  }
  // both vertical lines
  for (; i < vert ; ++i) {
    sprintf(position, place_, w->y_beg + i  , w->x_beg);
    move(1, position);
    if (w != &w2) {
      write(1, ARRAY[cont_1], strlen(ARRAY[cont_1]));
    } else {
      write(1, ARRAY[cont_1], strlen(ARRAY[cont_1]));
      sprintf(position, place_, w->y_beg + i , w->x_beg + horiz);
      move(1, position);
      write(1, ARRAY[cont_1], strlen(ARRAY[cont_1]));
    }
  }
  // lower left corner
  sprintf(position, place_, vert + w->y_beg, w->x_beg );
  move(1, position);
  if (w == &w2) {
    write(1, ARRAY[cont_8], strlen(ARRAY[cont_8]));
  } else {
    write(1, ARRAY[cont_3], strlen(ARRAY[cont_3]));
  }
  // lower horizontal line
  sprintf(position, place_, vert + w->y_beg, w->x_beg + 1);
  move(1, position);
  for (j = 0; j < horiz - 1; ++j) {
    write(1, ARRAY[cont_0], strlen(ARRAY[cont_0]));
  }
  // lower right corner
  if (w == &w2) {
    sprintf(position, place_, vert + w->y_beg, w->x_beg + horiz);
    move(1, position);
    write(1, ARRAY[cont_5], strlen(ARRAY[cont_5]));
  }
  write(1, fg_reset, sizeof(fg_reset));
  sprintf(position, place_, vert / 2, w->x_beg + 1);
  move(1, position);
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
  char pos_place_[sizeof(place_) + 1];

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
  sprintf(attributes_num, ATTR_N, attributes.n_elements);
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

  char pos_c[strlen(place_)];
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
  sprintf(attr_arr, "attr_arr: %s", attributes.paths[0]);
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
