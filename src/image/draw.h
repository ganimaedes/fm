#ifndef DRAW_H
#define DRAW_H
#include "props.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <signal.h>
#include <execinfo.h>
#include <errno.h>

static volatile sig_atomic_t xwindow_created = 0;
static volatile sig_atomic_t resized = 0;
Window top_window; // term_window

void set_window_properties(Display* dsp, Window win);
void map_notify_handler(XEvent local_event, Display* display);
void check_event3(Display *display, XEvent *local_event, Window *img_window, Atom_Prop *atom_prop);
int processEvent3(Display *display,
                  Window *img_window,
                  int width,
                  int height,
                  Atom WM_message[2],
                  int *nth_call_to_process_event_function, Atom_Prop *atom_prop);
Window get_top_window(Display* d, Window start);
Window get_focus_window(Display* d);
void *detect_keypress(void *arg);
void close_xwindow(void *arg);
double fix_factor_to_fit_inside_window(Image *img, int width, int height);
#if defined(WITH_STBI)
void resize_image();
int load_stbi();
void get_bpl();
#else
XImage *CreateTrueColorImage(Display *display,
                             Visual *visual,
                             unsigned char *image,
                             int width,
                             int height);
#endif // WITH_STBI
void *openx(void *arg);
int set_image(char *path, STAT_INFO *info_file);
//int set_img(char *path, STAT_INFO *info_file);
int set_img(char *path);
void init_win_change_signal();

#endif // DRAW_H
