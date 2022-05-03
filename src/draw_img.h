#ifndef DRAW_IMG_H
#define DRAW_IMG_H
#include "props.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <signal.h>
#include <execinfo.h>
#include <errno.h>

void set_window_properties(Display* dsp, Window win);
void map_notify_handler(XEvent local_event, Display* display);
void check_event(Display *display, XEvent *local_event, Window *img_window, Atom_Prop *atom_prop);
int processEvent(Display *display,
                  Window *term_window,
                  Window *img_window,
                  XImage *ximage,
                  int width,
                  int height,
                  Atom WM_message[2],
                  int *nth_call_to_process_event_function, Atom_Prop *atom_prop);
Window get_top_window(Display* d, Window start);
Window get_focus_window(Display* d);
Window get_toplevel_parent(Display * display, Window window);
void *detect_keypress(void *arg);
void closex(void *arg);
double fix_factor_to_fit_inside_window(Image *img, int width, int height);
#if defined(WITH_STBI)
int load_stbi();
void put_image(Window *img_window, int x_px, int y_px);
#else
XImage *CreateTrueColorImage(Display *display,
                             Visual *visual,
                             unsigned char *image,
                             int width,
                             int height);
#endif // WITH_STBI
void *openx(void *arg);
//int set_img(char *path);
int set_img(char *path, STAT_INFO *info_file);

#endif // DRAW_IMG_H
