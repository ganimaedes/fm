#ifndef IMG_H
#define IMG_H
// xtruss -e events -C ./min -id 0x<WINDOW_ID> <IMAGE_PATH> 0.5 980 50
// xgc --version
// xdpyinfo
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

#define ATOM(a) XInternAtom(foreground_dpy, #a, False)

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
  int screen;
  Window foreground_win;
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
Window select_args(int *rargc, char **argv);
Window get_child_window(Display *display, Window window);
Window get_toplevel_parent(Display * display, Window window);
void handlern(int sig);
void create_window(Win *win, Window *root, int x_px, int y_px, Image *img);
void grab_keys(Win *win);
void put_image(Win *win, Image *img, int x_px, int y_px);
Window get_top_window(Display* d, Window start);
Window get_focus_window(Display* d);
//int set_img(int argc, char **argv);
int set_img(int argc, char *prog_name, Window window_in, char *path, double factor_in, int y_pos_in, int x_pos_in);

#endif // IMG_H
