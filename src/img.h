#ifndef IMG_H
#define IMG_H
// xtruss -e events -C ./min -id 0x<WINDOW_ID> <IMAGE_PATH> 0.5 980 50
// xgc --version
// xdpyinfo
#include "scr.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
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
#if defined(SHOW_ATOMS)
#include "debug.h"
#endif // SHOW_ATOMS

#define MAXSTR      500
#ifndef MIN
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#endif

static Display *foreground_dpy = NULL;
static sig_atomic_t volatile window_unmapped = 0;
static sig_atomic_t volatile window_remapped = 0;

#ifndef TRUE
#define TRUE  1
#endif // TRUE
#ifndef FALSE
#define FALSE 0
#endif // FALSE
static Bool xerror = FALSE;

static unsigned char SIGNATURE_PNG[9] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00 };
static unsigned char SIGNATURE_JPG[11] = { 0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00 };
static unsigned char SIGNATURE_GIF[3] = { 0x47, 0x49, 0x46 };
static size_t SIZE_JPEG_ARRAY = sizeof(SIGNATURE_JPG) / sizeof(*SIGNATURE_JPG);
static size_t SIZE_PNG_ARRAY = sizeof(SIGNATURE_PNG) / sizeof(*SIGNATURE_PNG);
static size_t SIZE_GIF_ARRAY = sizeof(SIGNATURE_GIF) / sizeof(*SIGNATURE_GIF);

#define ATOM(a) XInternAtom(foreground_dpy, #a, False)

static volatile sig_atomic_t counter_position = 0;
static volatile sig_atomic_t x_pos_debug = 1;
static volatile sig_atomic_t y_pos_debug = 1;

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

static char formatting_buffer[MAXSTR + 100];
static char formatting_copy[MAXSTR + 100];

typedef struct _Atom_Prop {
  char *status;
  int total_len;
} Atom_Prop;

static Window target_win = 0;

Window tmp_window;

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
} Image;

void close_display();
void error(const char *msg, ...);
static void setup_mapping(void);
static char *format_atom(Atom atom);
void init_array(Atom_Prop *atom_prop, size_t inital_size);
int add_element(Atom_Prop *atom_prop, char *element);
static void display_property2(Atom_Prop *atom_prop, Properties *properties, int use_dyn);
static long extract_value(const char **data, int *length, int size);
static Properties *break_down_property(const char *data, int length, Atom type, int size);
static void show_prop2(Atom_Prop *atom_prop, const char *prop, Window *top_win, int use_dyn);
static void show_properties(Atom_Prop *atom_prop, Window *top_win, int use_dyn);
Window get_toplevel_parent(Display * display, Window window);
void handlern(int sig);
void create_window(Win *win, Window *root, int x_px, int y_px, Image *img, char *path);
void grab_keys(Win *win);
void put_image(Win *win, Image *img, int x_px, int y_px);
Window get_top_window(Display* d, Window start);
Window get_focus_window(Display* d);
int x_error_handler(Display* dpy, XErrorEvent* pErr);
int check_if_key_press2(InfoKeyPresses *info);
int was_it_auto_repeat(Display * d, XEvent * event, int current_type, int next_type);
int process_event3(GC *gc,
                   Window *top_window,
                   Atom *wmDeleteMessage,
                   Win *w,
                   Atom_Prop *atom_prop,
                   Image *img,
                   KeyCode *key,
                   InfoKeyPresses *info);
unsigned long set_img(char *path, InfoKeyPresses *info);

#endif // IMG_H
