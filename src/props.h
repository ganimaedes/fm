#ifndef PROPS_H
#define PROPS_H

#include "scr.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <termios.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/select.h>
#include <stdint.h>
#include <pthread.h>
#include <limits.h>

#define MAXSTR      500
#ifndef MIN
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#endif

// Concatenate preprocessor tokens A and B without expanding macro definitions
// (however, if invoked from a macro, macro arguments are expanded).
//
#define PPCAT_NX(A, B) A ## B

// Concatenate preprocessor tokens A and B after macro-expanding them.
//
#define PPCAT(A, B) PPCAT_NX(A, B)

// Turn A into a string literal without expanding macro definitions
// (however, if invoked from a macro, macro arguments are expanded).
#define STRINGIZE_NX(A) #A

// Turn A into a string literal after macro-expanding it.
//
#define STRINGIZE(A) STRINGIZE_NX(A)

// https://stackoverflow.com/questions/8257714/how-to-convert-an-int-to-string-in-c
#define ENOUGH_INT ((CHAR_BIT * sizeof(int)) / 3 + 2)
#define ENOUGH_STR ((CHAR_BIT * sizeof(char)) / 3 + 2)
#define ENOUGH_UINT ((CHAR_BIT * sizeof(unsigned)) / 3 + 2)
#define ENOUGH_LUINT ((CHAR_BIT * sizeof(unsigned long)) / 3 + 2)

#ifndef TRUE
#define TRUE  1
#endif // TRUE
#ifndef FALSE
#define FALSE 0
#endif // FALSE
//int quit = FALSE;

//#define _file_descriptor           1

#ifndef __file_descriptor
#define __file_descriptor 1
#endif // __file_descriptor
#define write_line_debug(fd, str) if (write((fd), (str), strlen(str)) < 0) { fprintf(stderr, "Error write\n"); exit(1); }

#define _LINENUMBER "%d"
static char _line_number[ENOUGH_INT];
#define __PRINTDEBUG do {                          \
  write_line_debug(__file_descriptor, __FILE__);     \
  write_line_debug(__file_descriptor, ":");          \
  write_line_debug(__file_descriptor, __func__);     \
  write_line_debug(__file_descriptor, ":");          \
  sprintf(_line_number, _LINENUMBER, __LINE__);    \
  write_line_debug(__file_descriptor, _line_number); \
  write_line_debug(__file_descriptor, "\n\t");       \
} while (0)

#define __uint "%u"
static char __uintnumber[ENOUGH_UINT];
#define write_int(num) do { sprintf(_line_number, _LINENUMBER, num); write_line_debug(__file_descriptor, _line_number); } while(0)
#define write_uint(num) do { sprintf(__uintnumber, __uint, num); write_line_debug(__file_descriptor, __uintnumber); } while(0)

#define __PRINTMAPINFO(__case) do { \
  printf("case %s:\n\tmapped = %d | raised = %d | just_unraised = %d | just_unmapped = %d\n", \
  (__case), mapped, raised, just_unraised, just_unmapped); \
} while(0)

#define __PRINTMAPINFO_N(__case) do { \
  printf("case %s:\n\tmapped = %d | raised = %d | just_unraised = %d | just_unmapped = %d | nth_call = %d\n", \
  (__case), mapped, raised, just_unraised, just_unmapped, *nth_call_to_process_event_function); \
} while(0)

#define __PRINTEVENTSELECTIONS do { \
  printf("\n\tconfig_check_event_mapped = %d, focusin_check_event_mapped = %d, focusout_check_event_mapped = %d\n", \
  config_check_event_mapped, focusin_check_event_mapped, focusout_check_event_mapped); \
} while(0)

#define __INVERTEVENTSELECTIONS do { \
  \
} while(0)

#define SEPARATOR write_line_debug(__file_descriptor, " | ")
#define LINE_CH   write_line_debug(__file_descriptor, "\n")
#define TAB   write_line_debug(__file_descriptor, "\t")

#define __PRINTMAPINFO_N2(__case) do { \
  write_line_debug(__file_descriptor, "case "); \
  write_line_debug(__file_descriptor, __case); \
  LINE_CH; TAB; \
  PRINTVALUE_1LINE(mapped); SEPARATOR; \
  PRINTVALUE_1LINE(raised); SEPARATOR; \
  PRINTVALUE_1LINE(just_unraised); SEPARATOR; \
  PRINTVALUE_1LINE(just_unmapped); SEPARATOR; \
  PRINTVALUE_1LINE(*nth_call_to_process_event_function); LINE_CH; \
} while(0)

#define __PRINTMAPINFO2(__case) do { \
  write_line_debug(__file_descriptor, "case "); \
  write_line_debug(__file_descriptor, __case); \
  LINE_CH; TAB; \
  PRINTVALUE_1LINE(mapped); SEPARATOR; \
  PRINTVALUE_1LINE(raised); SEPARATOR; \
  PRINTVALUE_1LINE(just_unraised); SEPARATOR; \
  PRINTVALUE_1LINE(just_unmapped); LINE_CH; \
} while(0)

#define PRINTVALUE_1LINE(value) do {                       \
  write_line_debug(__file_descriptor, STRINGIZE_NX(value)); \
  write_line_debug(__file_descriptor, " = ");               \
  sprintf(_line_number, _LINENUMBER, (value));             \
  write_line_debug(__file_descriptor, (_line_number));      \
} while(0)

#define PRINTSTRING_1LINE(value) do {                      \
  write_line_debug(__file_descriptor, STRINGIZE_NX(value)); \
  write_line_debug(__file_descriptor, ": ");                \
  write_line_debug(__file_descriptor, (value));             \
} while(0)

#define PRINTVALUE(value) do {                             \
  write_line_debug(__file_descriptor, STRINGIZE_NX(value)); \
  write_line_debug(__file_descriptor, " = ");               \
  sprintf(_line_number, _LINENUMBER, (value));             \
  write_line_debug(__file_descriptor, (_line_number));      \
  write_line_debug(__file_descriptor, ("\n\t"));            \
} while(0)

#define lhex__ "%lx"
static char __lxnumber[ENOUGH_LUINT];
#define PRINTVALUE_UNSIGNED(value) do {                    \
  write_line_debug(__file_descriptor, STRINGIZE_NX(value)); \
  write_line_debug(__file_descriptor, " = 0x");             \
  sprintf(__lxnumber, lhex__, (value));                    \
  write_line_debug(__file_descriptor, (__lxnumber));        \
  write_line_debug(__file_descriptor, ("\n"));              \
} while(0)

#define PRINTSTRING(value) do {                            \
  write_line_debug(__file_descriptor, STRINGIZE_NX(value)); \
  write_line_debug(__file_descriptor, ": ");                \
  write_line_debug(__file_descriptor, (value));             \
  write_line_debug(__file_descriptor, ("\n\t"));            \
} while(0)

#define PRINT(msg) do {                                          \
  fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__); \
  fprintf(stdout, "%s\n", (msg));                                \
  exit(1);                                                       \
} while (0)

#define comma write_line_debug(__file_descriptor, ":")

#define PRINTWRITE(msg) do {                                        \
  write_line_debug(__file_descriptor, __FILE__); comma;              \
  write_line_debug(__file_descriptor, __func__); comma;              \
  sprintf(_line_number, _LINENUMBER, __LINE__);                     \
  write_line_debug(__file_descriptor, (_line_number)); LINE_CH; TAB; \
  write_line_debug(__file_descriptor, msg); LINE_CH;                 \
  if (quit) exit(1);                                                \
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

/*
typedef struct _Array {
  Window *windows;
  unsigned int n_elements;
  unsigned int capacity;
} Array;
*/

typedef struct _Properties {
  int         n_elements;
  int         capacity;
  long        value;
} Properties;

static Properties *property_formats = NULL;

typedef struct _PropertyRec {
  const char *name;
  Atom	atom;
} PropertyRec;

static PropertyRec windowPropTable[] = { { "ATOM", XA_ATOM } };

typedef struct _Atom_Prop {
  char *status;
  int total_len;
} Atom_Prop;

static unsigned char SIGNATURE_PNG[9] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00 };
static unsigned char SIGNATURE_JPG[11] = { 0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00 };
static unsigned char SIGNATURE_GIF[3] = { 0x47, 0x49, 0x46 };
static size_t SIZE_JPEG_ARRAY = sizeof(SIGNATURE_JPG) / sizeof(*SIGNATURE_JPG);
static size_t SIZE_PNG_ARRAY = sizeof(SIGNATURE_PNG) / sizeof(*SIGNATURE_PNG);
static size_t SIZE_GIF_ARRAY = sizeof(SIGNATURE_GIF) / sizeof(*SIGNATURE_GIF);

/*
static char *TYPE[] = {
  "j", // JPEG JIFF
  "p", // PNG
  "g", // GIF
  "d", // PDF
  "o"  // FOLDER
};

typedef struct _STAT_INFO {
  unsigned short width;
  unsigned short height;
  unsigned long file_len;
  char *file_name;
  FILE *file;
  unsigned char *data;
} STAT_INFO;
*/

typedef struct _Win {
  Window grab_window;
  KeyCode keycode_dn;
  KeyCode keycode_up;
  KeyCode keycodes[10];
  int keycode_end_pressed;
  int keycode_beg_pressed;
  int keycode_bckspce_pressed;
  int keycode_dn_pressed;
  int keycode_up_pressed;
  int keycode_page_dn_pressed;
  int keycode_page_up_pressed;
  int keycode_escape_pressed;
  int keycode_copy_pressed;


  int screen;
  Window foreground_win;
  Window background_win;
  Atom window_type;
  long value;
} Win;

typedef struct _Image {
  int depth;
  int bpl;
  int width;
  int height;
  int new_width;
  int new_height;
  int n;
  GC gc;
  uint8_t *data_resized;
  uint8_t *data;
  XImage *ximage;
  size_t size;
  char *path;

} Image;

typedef struct keyQueue {
  struct keyQueue *next;
  char key;
} keyQueue_t;

typedef struct ThreadInfo {
  pthread_mutex_t kqmutex; // protects key queue from race condition between threads
  keyQueue_t kqhead;       // input keys queued to this thread
  char *keys;              // keys this thread responds to
} threadInfo_t;

#define ONECHAR "%c"
char onechar[sizeof(ONECHAR)];
#define write_char(ch) do { sprintf(onechar, ONECHAR, ch); if (write(1, (onechar), strlen(onechar)) < 0) { exit(1); } } while(0)
//#define write_line(str) if (write(1, (str), strlen(str)) < 0) { exit(1); }

static XWindowAttributes xwa_image;
static XWindowAttributes xwa;
Win __window;
Window img_window;
XImage *ximage;
Display *display;

static Display *foreground_dpy = NULL;
static char formatting_buffer[MAXSTR + 100];
static char formatting_copy[MAXSTR + 100];
static Window target_win = 0;
static Window tmp_window;
static volatile sig_atomic_t image_cp_signal = 0;
//struct termios term, oterm;

void close_display();
void error(const char *msg, ...);
void setup_mapping(Display *dis);
char *format_atom(Atom atom, Display *dis);
void init_array(Atom_Prop *atom_prop, size_t inital_size);
int add_element(Atom_Prop *atom_prop, char *element);
void display_property2(Atom_Prop *atom_prop, Properties *properties, Display *dis, int use_dyn);
long extract_value(const char **data, int *length, int size);
Properties *break_down_property(const char *data, int length, Atom type, int size);
void show_prop2(Atom_Prop *atom_prop, const char *prop, Window *top_win, Display *dis, int use_dyn);
void show_properties(Atom_Prop *atom_prop, Window *top_win, Display *dis, int use_dyn);
Window Select_Window_Args(int *rargc, char **argv);

#endif // PROPS_H
