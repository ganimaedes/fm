#include "img.h"
#include "array.h"
#include "scr.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <unistd.h>
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

void close_display()
{
  if (foreground_dpy == NULL) {
    return;
  }
  XCloseDisplay(foreground_dpy);
  foreground_dpy = NULL;
}

void error(const char *msg, ...)
{
  va_list args = { 0 };
  fflush(stdout);
  fflush(stderr);
  fprintf(stderr, "Error: ");
  va_start(args, msg);
  vfprintf(stderr, msg, args);
  va_end(args);
  fprintf(stderr, "\n");
  close_display();
  exit(EXIT_FAILURE);
}

static void setup_mapping(void)
{
  PropertyRec *p = windowPropTable;
  int n = sizeof(windowPropTable) / sizeof(PropertyRec);
  for (; --n >= 0; ++p) {
    if (!p->atom) {
      p->atom = XInternAtom(foreground_dpy, p->name, True);
      if (p->atom == None) {
        continue;
      }
    }
    if (!property_formats) {
      property_formats = calloc(n + 1, sizeof *property_formats);
      if (!property_formats) {
        error("Error calloc property_formats");
      }
      property_formats->capacity       = n + 1;
      property_formats->n_elements     = 0;
      property_formats->value          = 0;
    }
    property_formats[property_formats->n_elements++].value = p->atom;
  }
}

static char *format_atom(Atom atom)
{
  char *name = NULL;
  name = XGetAtomName(foreground_dpy, atom);
  if (!name) {
    snprintf(formatting_buffer, sizeof(formatting_buffer),
        "undefined atom # 0x%lx", atom);
  } else {
    int namelen = strlen(name);
    if (namelen > MAXSTR) {
      namelen = MAXSTR;
    }
    memcpy(formatting_buffer, name, namelen);
    formatting_buffer[namelen] = '\0';
    memcpy(formatting_copy, name, namelen);
    formatting_copy[namelen] = '\0';
    XFree(name);
  }
  return formatting_buffer;
}

void init_array(Atom_Prop *atom_prop, size_t inital_size)
{
  atom_prop->status = malloc((inital_size + 1) * sizeof *atom_prop->status);
  if (!atom_prop->status) {
    fprintf(stderr, "Error calloc atom->status\n");
  }
  atom_prop->total_len = 0;
}

int add_element(Atom_Prop *atom_prop, char *element)
{
  size_t len_element = strlen(element);
  if (len_element > 0) {
    atom_prop->total_len += len_element;
    void *tmp = NULL;
    tmp = realloc(atom_prop->status, (atom_prop->total_len + 1) * sizeof(char *));
    if (!tmp) {
      fprintf(stderr, "Error realloc atom->status\n");
      return 0;
    }
    atom_prop->status = tmp;
    memcpy(&atom_prop->status[atom_prop->total_len - len_element], element, len_element);
    atom_prop->status[atom_prop->total_len] = '\0';
#if defined(SHOW_ATOMS)
    //sprintf(del_in_debug, del_debug, 100 - 2);
    //fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
    //if (++n_times_add_element_called == atom_prop->total_len - 1) {
      //__PRINTDEBUG;
      //PRINTDEBUG("");

      //    printf("len_element %lu, atom_prop->total_len = %d\natom_prop->status[%d]: %s\n",
      //        len_element, atom_prop->total_len, atom_prop->total_len, atom_prop->status);
      //printTTYSTR(2, 1, "len_element"); printTTYLONGUNSIGNED(2, strlen("len_element"), len_element);
      //printTTYSTR(2, 1, "atom_prop->total_len"); printTTYINT(1, strlen("atom_prop->total_len"), atom_prop->total_len);
      //printTTYSTR(3, 1, "atom_prop->status["); printTTYINT(1, strlen("atom_prop->status["), atom_prop->total_len);
      //printTTYSTR(3, strlen("atom_prop->status[") + 3, "]: "); printTTYSTR(3, strlen("atom_prop->status[]") + 3, atom_prop->status);
    //}
#endif
    return 1;
  }
  return 0;
}

static void display_property2(Atom_Prop *atom_prop, Properties *properties, int use_dyn)
{
  long i          = 0;
  char *name      = NULL;
#if defined(V_DEBUG)
  fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
  printf("properties->n_elements = %d\n", properties->n_elements);
#endif
  if (use_dyn) {
    init_array(atom_prop, 30);
  }
  for (; i < properties->n_elements; ++i) {
    name = XGetAtomName(foreground_dpy, properties[i].value);
#if defined(V_DEBUG)
    fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
    fprintf(stdout, "name: %s\n", name);
#endif
    if (use_dyn) {
      add_element(atom_prop, name);
    }
    else {
      int namelen = strlen(name);
      memcpy(formatting_buffer, name, namelen);
      formatting_buffer[namelen] = '\0';
    }
    if (name != NULL && counter_position < 1) {
      ++y_pos_debug;
      //__PRINTDEBUG;
      ++y_pos_debug;
      //printTTYSTR(y_pos_debug, x_pos_debug, name);
      ++counter_position;
    }
    XFree(name);
  }
}

static long extract_value(const char **data, int *length, int size)
{
  long value;
  if (size != 32) {
    error("size isn't 32");
  }
  value = *(const long *) *data;
  *data += sizeof(long);
  *length -= sizeof(long);
#if defined(V_DEBUG)
  fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
  fprintf(stdout, "value = %lu, **data = %c, *length = %d\n", value, **data, *length);
#endif
  return value;
}

static Properties *break_down_property(const char *data, int length, Atom type, int size)
{
  int n_properties_to_add = 0;
  int length_cpy          = length;
  if (size == 32) {
    while (length_cpy >= size / 8) { ++n_properties_to_add; length_cpy -= sizeof(long); }
  }
#if defined(V_DEBUG)
  fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
  fprintf(stdout, "n_properties_to_add = %d | length = %d\n", n_properties_to_add, length);
#endif

  Properties *properties = malloc(n_properties_to_add * sizeof *properties);
  if (!properties) {
    error("Error malloc properties");
  }
  properties->capacity   = n_properties_to_add;
  properties->n_elements = 0;
  properties->value      = 0;

  while (length >= size / 8 && properties->n_elements <= properties->capacity) {
    properties[properties->n_elements++].value = extract_value(&data, &length, size);
  }
  return properties;
}


static void show_prop2(Atom_Prop *atom_prop, const char *prop, Window *top_win, int use_dyn)
{
  int size                  = 0;
  long length               = 0;
  unsigned long nitems      = 0;
  unsigned long nbytes      = 0;
  unsigned long bytes       = 0;
  int status                = 0;
  unsigned char *data       = NULL;
  Atom type;
  Atom atom  = XInternAtom(foreground_dpy, prop, True);
  if (atom == None) {
    printf(":  no such atom on any window.\n");
    return;
  }
  status = XGetWindowProperty(foreground_dpy, *top_win, atom, 0, (MAXSTR + 3) / 4,
                              False, AnyPropertyType, &type,
                              &size, &nitems, &bytes,
                              &data);
  if (status == BadWindow || status != Success) {
    error("window id # 0x%lx does not exists!", target_win);
  }
  if (size == 32) {
    nbytes = sizeof(long);
  } else {
    error("actual_format ie size: %d isn't 32", size);
  }
  length = MIN(nitems * nbytes, MAXSTR);
  if (!size) {
    puts(":  not found.");
    return;
  }
  Properties *properties = break_down_property((const char *)data, (int)length, type, size);
  display_property2(atom_prop, properties, use_dyn);
#if defined(V_DEBUG)
//  fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
//  printf("tmp_window = 0x%lx\n\t", *top_win);
//  printf("atom_prop->status = %s\n", atom_prop->status);
#endif
  XFree(data);
  data = NULL;
  free(properties);
  properties = NULL;
}


static void show_properties(Atom_Prop *atom_prop, Window *top_win, int use_dyn)
{
  Atom *atoms     = NULL;
  char *name      = NULL;
  int count, i;
  if (target_win != -1 || *top_win != -1) {
    atoms = XListProperties(foreground_dpy, *top_win, &count);
    for (i = 0; i < count; ++i) {
#if defined(V_DEBUG)
//      fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
//      printf("tmp_window = 0x%lx\n\t", *top_win);
//      printf("atom = %lu\n", atoms[i]);
#endif
      name = format_atom(atoms[i]);
#if defined(V_DEBUG)
      printf("name = \t%s\n", name);
#endif
      if (strstr(name, "_NET_WM_STATE") /* || strstr(name, "_NET_WM_NAME") */) {
        show_prop2(atom_prop, name, top_win, use_dyn);
        break;
      }
    }
    XFree(atoms);
  }
}

Window get_toplevel_parent(Display * display, Window window)
{
  Window parent;
  Window root;
  Window *children;
  unsigned int num_children;

  for (;;) {
    if (!XQueryTree(display, window, &root, &parent, &children, &num_children))
      fprintf(stderr, "XQueryTree error: %s\n", __func__);
    if (children)
      XFree(children);
    if (window == root || parent == root)
      return window;
    else
      window = parent;
  }
}

void handlern(int sig)
{
  void *array[20] = { NULL };
  size_t size = backtrace(array, 20);
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  close_display();
  exit(1);
}

double loop_fix(Image *img,
                double *width,
                double *height,
                double temp_width,
                double temp_height,
                int *width_taken,
                double factor,
                double upper_lower_limit,
                int *image_width,
                int *image_height)
{
  if (*image_width > (*width / 2)) {
    temp_width = (double)(*width / 2);
    factor = temp_width / *image_width;
    *image_width *= factor;
    *width_taken = 1;
  } else if ((double)(*image_height) > (double)(*height - upper_lower_limit)) {
    factor = temp_height / (double)(*image_height);
    *image_height *= factor;
  }

  if (*image_width > (*width / 2)) {
    factor = 1 / (*image_width / (*width / 2));
    printf("factor = %f  ", factor);
  }
  return factor;
}

double fix_factor_to_fit_inside_window(Image *img, int width, int height)
{
  double factor = 1.0;
  double upper_lower_limit = 190.0;
  if (img->width > (width / 2) - 50) {
    factor = 1.0 / (((double)img->width) / ((double)((width / 2) - 50)));
  }
  double value = (double)((double)height - upper_lower_limit) / img->height;
  return (value < factor) ? value : factor;
}

void create_window(Win *win, Window *root, int x_px, int y_px, Image *img, char *path)
{

  int width = DisplayWidth(foreground_dpy, win->screen);
  int height = DisplayHeight(foreground_dpy, win->screen);
  // Load image into data variable
  img->data = stbi_load(path, &(img->width), &(img->height), &(img->n), 4);

  double factor = 1.0;

  XWindowAttributes xwa_image;
  xwa_image.override_redirect = TRUE;
  XWindowAttributes xwa;
  if (!XGetWindowAttributes(foreground_dpy, win->background_win, &xwa)) {
    PRINT("Error XGetWindowAttributes\n");
  }
  factor = fix_factor_to_fit_inside_window(img, xwa.width, xwa.height);
  img->new_width = img->width * factor;
  img->new_height = img->height * factor;

  // Allocate Memory for data_resized variable with new_width & new_height
  int new_width = (int)img->new_width;
  int new_height = (int)img->new_height;
  img->data_resized = malloc((new_width * new_height * 4) * sizeof *img->data_resized);
  if (img->data_resized == NULL) {
    fprintf(stderr, "Error malloc data_resized.\n");
    exit(1);
  }

  // Resize image from data to data_resized
  stbir_resize_uint8(img->data, img->width, img->height,
                     0, img->data_resized, img->new_width, img->new_height, 0, 4);

  int i = 0,
      len = img->new_width * img->new_height;
  unsigned int *dp = (unsigned int *)img->data_resized;
  for (; i < len; ++i) {
    dp[i] = (dp[i] & 0xFF00FF00) | ((dp[i] >> 16) & 0xFF) | ((dp[i] << 16) & 0xFF0000);
  }

  //img->img_size = sizeof(img->data_resized);
  //img->size = img->new_height * img->new_width * 4;

  if (!XGetWindowAttributes(foreground_dpy, win->background_win, &xwa)) {
    fprintf(stderr, "Error XGetWindowAttributes\n");
  }
  //printf("size = %lu", img->size);


#if defined(V_DEBUG_POSITION)
  fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
  printf("root = 0x%lx\n\t", *root);
#endif
  win->grab_window = *root;
#if defined(V_DEBUG_POSITION)
  printf("win->grab_window = 0x%lx\n\t", win->grab_window);
#endif
   win->keycode_dn = XKeysymToKeycode(foreground_dpy, XK_Down); // XK_Down = 0xff54
   win->keycode_up = XKeysymToKeycode(foreground_dpy, XK_Up);
   win->keycodes[0] = XKeysymToKeycode(foreground_dpy, XK_End);
   win->keycodes[1] = XKeysymToKeycode(foreground_dpy, XK_Begin);
   win->keycodes[2] = XKeysymToKeycode(foreground_dpy, XK_BackSpace);
   win->keycodes[3] = XKeysymToKeycode(foreground_dpy, XK_Page_Down);
   win->keycodes[4] = XKeysymToKeycode(foreground_dpy, XK_Page_Up);
   win->keycodes[5] = XKeysymToKeycode(foreground_dpy, XK_Escape);
   win->keycodes[6] = XKeysymToKeycode(foreground_dpy, XK_c);

  win->screen = DefaultScreen(foreground_dpy);
#if defined(V_DEBUG_POSITION)
  printf("win->screen = %d\n\t", win->screen);
  printf("x_px = %d | y_px = %d\n\t", x_px, y_px);
#endif
  int x_dist = 100;
  int center_in_second_window_dist = (xwa.width) / 4;
  win->foreground_win = XCreateSimpleWindow(foreground_dpy, *root, ((xwa.width / 2) + xwa.x + center_in_second_window_dist - img->new_width / 2) - 10,
                                              xwa.y + 49,
                                              img->new_width, img->new_height, 0,
                                              BlackPixel(foreground_dpy, win->screen),
                                              WhitePixel(foreground_dpy, win->screen));
#if defined(V_DEBUG_POSITION)
  printf("win->foreground_win = 0x%lx\n", win->foreground_win);
#endif
  win->window_type = XInternAtom(foreground_dpy, "_NET_WM_WINDOW_TYPE", False);
  win->value = XInternAtom(foreground_dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
  //printf("value = 0x%lx\n", value);
  //printf("char value: %s\n", (unsigned char *)value);
  XChangeProperty(foreground_dpy,
                  win->foreground_win,
                  win->window_type,
                  XA_ATOM,
                  32,
                  PropModeReplace,
                  (unsigned char *)&(win->value),
                  1);
  XSelectInput(foreground_dpy,
               win->foreground_win,
               KeyPressMask | PropertyChangeMask | StructureNotifyMask);

  ttymode_reset(ECHO, 1);
  //printTTY_UL(xwa.height - 1, 1, img->size / 1000);
  //printTTY_filesize(1, xwa.height - 2, ((double)img->size / 1000.0));
  print_tty_filesize3(1, xwa.height - 2, ((double)img->size / 1000.0));
  ttymode_reset(ECHO, 0);
}

void grab_keys(Win *win)
{
  unsigned int   modifiers       = 0;
  int            owner_events    = 0;
  int            pointer_mode    = GrabModeAsync;
  int            keyboard_mode   = GrabModeAsync;


  //XGrabKeyboard(foreground_dpy, win->grab_window, True, GrabModeAsync, GrabModeAsync, CurrentTime);

  XGrabKey(foreground_dpy, win->keycodes[6], modifiers,
           win->grab_window, owner_events, pointer_mode,
           keyboard_mode);
  XGrabKey(foreground_dpy, win->keycode_dn, modifiers,
           win->grab_window, owner_events, pointer_mode,
           keyboard_mode);
// https://stackoverflow.com/a/29001687
// https://stackoverflow.coa/4037579
  XGrabKey(foreground_dpy, win->keycode_dn, Mod2Mask,
           win->grab_window, owner_events, pointer_mode,
           keyboard_mode);
 // https://stackoverflow.com/a/29001687 https://stackoverflow.com/a/4037579
  XGrabKey(foreground_dpy, win->keycode_up, modifiers,
           win->grab_window, owner_events, pointer_mode,
           keyboard_mode);
  XGrabKey(foreground_dpy, win->keycode_up, Mod2Mask,
           win->grab_window, owner_events, pointer_mode,
           keyboard_mode);
// https://stackoverflow.com/a/29001687 https://stackoverflow.com/a/4037579
  XGrabKey(foreground_dpy, win->keycodes[0], modifiers, // XK_End
           win->grab_window, owner_events, pointer_mode,
           keyboard_mode);
//  XGrabKey(foreground_dpy, win->keycodes[0], Mod2Mask,
//           win->grab_window, owner_events, pointer_mode,
//           keyboard_mode);
//  XGrabKey(foreground_dpy, win->keycodes[1], modifiers, // XK_Begin
//           win->grab_window, owner_events, pointer_mode,
//           keyboard_mode);
//  XGrabKey(foreground_dpy, win->keycodes[1], Mod2Mask,
//           win->grab_window, owner_events, pointer_mode,
//           keyboard_mode);
  XGrabKey(foreground_dpy, win->keycodes[2], modifiers, // XK_BackSpace
           win->grab_window, owner_events, pointer_mode,
           keyboard_mode);
//

  XGrabKey(foreground_dpy, win->keycodes[2], Mod2Mask,
           win->grab_window, owner_events, pointer_mode,
           keyboard_mode);
  //XGrabKeyboard(foreground_dpy, win->grab_window, True, GrabModeAsync, GrabModeAsync, CurrentTime);

  XGrabKey(foreground_dpy, win->keycodes[3], modifiers, // XK_Page_Down
           win->grab_window, owner_events, pointer_mode,
           keyboard_mode);
  XGrabKey(foreground_dpy, win->keycodes[3], Mod2Mask, // XK_Page_Down
           win->grab_window, owner_events, pointer_mode,
           keyboard_mode);
/*
  XGrabKey(foreground_dpy, win->keycodes[3], Mod1Mask, // XK_Page_Down
           win->grab_window, owner_events, pointer_mode,
           keyboard_mode);
*/

  XGrabKey(foreground_dpy, win->keycodes[4], Mod2Mask, // XK_Page_Up
           win->grab_window, owner_events, pointer_mode,
           keyboard_mode);
/*
  XGrabKey(foreground_dpy, win->keycodes[4], modifiers, // XK_Page_Up
           win->grab_window, owner_events, pointer_mode,
           keyboard_mode);
*/
  XGrabKey(foreground_dpy, win->keycodes[5], modifiers, // XK_Escape
           win->grab_window, owner_events, pointer_mode,
           keyboard_mode);
}

void put_image(Win *win, Image *img, int x_px, int y_px)
{
  // https://stackoverflow.com/questions/11069666/cannot-get-xcreatesimplewindow-to-open-window-at-the-right-position
  XSizeHints    my_hints = {0};
  my_hints.flags  = PPosition | PSize;     /* specify position and size */
  my_hints.x      = x_px;                  /* origin and size coords I want */
  my_hints.y      = y_px;
  my_hints.width  = img->new_width;
  my_hints.height = img->new_height;
  XSetNormalHints(foreground_dpy, win->foreground_win, &my_hints);  /* Where new_window is the new window */

  XMapWindow(foreground_dpy, win->foreground_win);
  img->bpl = 0;
  //nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);
  //nanosleep((const struct timespec[]){{0, 9000000L}}, NULL);

  img->depth = DefaultDepth(foreground_dpy, win->screen);
  switch (img->depth) {
    case 24:          img->bpl = 4; break;
    case 16: case 15: img->bpl = 2; break;
    default:          img->bpl = 1; break;
  }
#if defined(V_DEBUG_POSITION)
  fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
  printf("img->depth = %d\n", img->depth);
#endif
}

// https://stackoverflow.com/questions/31535560/xlib-and-firefox-behavior
// https://stackoverflow.com/questions/31535560/xlib-and-firefox-behavior
// https://github.com/leahneukirchen/cwm/blob/linux/xevents.c
// https://gist.github.com/kui/2622504
// get the top window.
// a top window have the following specifications.
//  * the start window is contained the descendent windows.
//  * the parent window is the root window.
Window get_top_window(Display* d, Window start)
{
  Window w = start;
  Window parent = start;
  Window root = None;
  Window *children;
  unsigned int nchildren;
  Status s;

  while (parent != root) {
    w = parent;
    s = XQueryTree(d, w, &root, &parent, &children, &nchildren);
    if (s) {
      XFree(children);
    }
    if (xerror) {
      printf("fail\n");
      exit(1);
    }
  }
  return w;
}

Window get_focus_window(Display* d)
{
  Window w;
  int revert_to;
  //printf("getting input focus window ... ");
  XGetInputFocus(d, &w, &revert_to); // see man
  if (xerror) {
    printf("fail\n");
    exit(1);
  } else if (w == None) {
    printf("no focus window\n");
    exit(1);
  } else {
    return w;
  }
  return w;
}

int x_error_handler(Display* dpy, XErrorEvent* pErr)
{
  printf("X Error Handler called, values: %d/%lu/%d/%d/%d\n",
         pErr->type,
         pErr->serial,
         pErr->error_code,
         pErr->request_code,
         pErr->minor_code );
  if (pErr->request_code == 33) {  // 33 (X_GrabKey)
    if(pErr->error_code == BadAccess) {
      printf("ERROR: A client attempts to grab a key/button combination already\n"
          "        grabbed by another client. Ignoring.\n");
      return 0;
    }
  }
  exit(1);  // exit the application for all unhandled errors.
  return 0;
}

// https://blog.robertelder.org/detect-keyup-event-linux-terminal
int was_it_auto_repeat(Display * d, XEvent * event, int current_type, int next_type)
{
  /*  Holding down a key will cause 'autorepeat' to send fake keyup/keydown events, but we want to ignore these: '*/
  if(event->type == current_type && XEventsQueued(d, QueuedAfterReading)){
    XEvent nev;
    XPeekEvent(d, &nev);
    return (nev.type == next_type && nev.xkey.time == event->xkey.time && nev.xkey.keycode == event->xkey.keycode);
  }
  return FALSE;
}

// https://opensource.apple.com/source/X11libs/X11libs-60/mesa/Mesa-7.8.2/src/glut/glx/glut_event.c.auto.html
/* Unlike XNextEvent, if a signal arrives,
   interruptibleXNextEvent will return (with a zero return
   value).  This helps GLUT drop out of XNextEvent if a signal
   is delivered.  The intent is so that a GLUT program can call 
   glutIdleFunc in a signal handler to register an idle func
   and then immediately get dropped into the idle func (after
   returning from the signal handler).  The idea is to make
   GLUT's main loop reliably interruptible by signals. */
static int
interruptibleXNextEvent(Display * dpy, XEvent * event)
{
  int fd_foreground = 0;
  fd_foreground     = ConnectionNumber(foreground_dpy);

  fd_set fds;
  int rc;

  /* Flush X protocol since XPending does not do this
     implicitly. */
  XFlush(foreground_dpy);
  for (;;) {
    if (XPending(foreground_dpy)) {
      XNextEvent(dpy, event);
      return 1;
    }
//#ifndef VMS
    /* the combination ConectionNumber-select is buggy on VMS. Sometimes it
     * fails. This part of the code hangs the program on VMS7.2. But even
     * without it the program seems to run correctly.
     * Note that this is a bug in the VMS/DECWindows run-time-libraries.
     * Compaq engeneering does not want or is not able to make a fix.
     * (last sentence is a quotation from Compaq when I reported the
     * problem January 2000) */
    FD_ZERO(&fds);
    //FD_SET(__glutConnectionFD, &fds);
    FD_SET(fd_foreground, &fds);
    rc = select(fd_foreground + 1, &fds, NULL, NULL, NULL);
    if (rc < 0) {
      if (errno == EINTR) {
        return 0;
      } else {
        error("select error.");
      }
    }
//#endif
  }
}

int process_event3(GC *gc,
                   Window *top_window,
                   Atom *wmDeleteMessage,
                   Win *w,
                   Atom_Prop *atom_prop,
                   Image *img,
                   KeyCode *key,
                   InfoKeyPresses *info)
{
  XEvent xe;
  XEvent ahead;
  XConfigureEvent cEvent;
  XNextEvent(foreground_dpy, &xe);
  XSelectInput(foreground_dpy, *top_window,
      KeyPressMask | KeyReleaseMask | ExposureMask| PropertyChangeMask | StructureNotifyMask | SubstructureRedirectMask | SubstructureNotifyMask);
/*
  XSelectInput(foreground_dpy, w->foreground_win,
      KeyPressMask | KeyReleaseMask | ExposureMask| PropertyChangeMask | StructureNotifyMask | SubstructureRedirectMask | SubstructureNotifyMask);
*/
  XWindowAttributes xwa;
  if (!XGetWindowAttributes(foreground_dpy, *top_window, &xwa)) {
    fprintf(stderr, "Error XGetWindowAttributes\n");
  }

  show_properties(atom_prop, &tmp_window, 1);
  if (strstr(atom_prop->status, "HIDDEN")) {
#if defined(V_DEBUG)
    fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
    printf("\t\t\t\t\t\t\tHIDDEN\n\n\n\n\n\n");
#endif
    XUnmapWindow(foreground_dpy, w->foreground_win);
    XSync(foreground_dpy, True);
    nanosleep((const struct timespec[]){{0, 9000000L}}, NULL);
    XUngrabKey(foreground_dpy, (long)XLookupKeysym(&xe.xkey, 0), 0, *top_window);
    window_unmapped = 1;
    window_remapped = 0;
  } else if (window_unmapped == 1) {
    XMapWindow(foreground_dpy, w->foreground_win);
    XSync(foreground_dpy, False);
    nanosleep((const struct timespec[]){{0, 9000000L}}, NULL);
    XSync(foreground_dpy, True);
#if defined(V_DEBUG_POSITION)
    printf("img.ximage[0] = %d\n", img.ximage->data[0]);
#endif
    nanosleep((const struct timespec[]){{0, 9000000L}}, NULL);
    XPutImage(foreground_dpy, w->foreground_win,
              *gc, img->ximage,
              0, 0,
              0, 0,
              img->new_width, img->new_height);
    XSync(foreground_dpy, False);
    window_remapped = 1;
    window_unmapped = 0;
  }
  else if (!strstr(atom_prop->status, "WM_STATE_FOCUSED")) {
/*
)
    XSelectInput(foreground_dpy, w->foreground_win,
        KeyPressMask | KeyReleaseMask | ExposureMask| PropertyChangeMask | StructureNotifyMask | SubstructureRedirectMask | SubstructureNotifyMask);
    XLowerWindow(foreground_dpy, w->foreground_win);
    XSelectInput(foreground_dpy, *top_window,
        KeyPressMask | KeyReleaseMask | ExposureMask| PropertyChangeMask | StructureNotifyMask | SubstructureRedirectMask | SubstructureNotifyMask);
*/
  }

  if (atom_prop->status) {
    free(atom_prop->status);
    atom_prop->status = NULL;
  }
  //XSelectInput(foreground_dpy, *top_window,
  //    KeyPressMask | KeyReleaseMask | ExposureMask| PropertyChangeMask | StructureNotifyMask | SubstructureRedirectMask | SubstructureNotifyMask);

  switch (xe.type) {
    case MapNotify:
      XSync(foreground_dpy, False);
      return 1;
      break;
    case ClientMessage:
      if (xe.xclient.message_type == *wmDeleteMessage) {
        return 0;
      }
    case ConfigureNotify:
        cEvent = xe.xconfigure;
      break;
    case KeyPress:
      if (info_key_presses.last_element_is_not_img) {
        info_key_presses.last_element_is_not_img = 0;
        break;
      }
/*
      if (img->height > 450 && img->width > 400) {
        break;
      }
*/
      //break;
    case KeyRelease: {
      // https://opensource.apple.com/source/X11libs/X11libs-60/mesa/Mesa-7.8.2/src/glut/glx/glut_event.c.auto.html

	  //if (window->ignoreKeyRepeat) {
//
	    if (info_key_presses.last_element_is_not_img && XEventsQueued(foreground_dpy, QueuedAfterReading)) {
	      XPeekEvent(foreground_dpy, &ahead);
	      if (ahead.type == KeyPress
	        && ahead.xkey.window == xe.xkey.window
	        && ahead.xkey.keycode == xe.xkey.keycode
	        && ahead.xkey.time == xe.xkey.time) {
            // Pop off the repeated KeyPress and ignore
            //   the auto repeated KeyRelease/KeyPress pair.

	        XNextEvent(foreground_dpy, &xe);
            XUngrabKey(foreground_dpy, (long)XLookupKeysym(&xe.xkey, 0), 0, *top_window);
            XKeyPressedEvent eventkey;


            //XUngrabKeyboard(foreground_dpy, CurrentTime);
            ++info->n_times_pressed;
            info_key_presses.last_element_is_not_img = 0;
            return 0;
	        //break;
	      }
	    }

	  //}
    // https://stackoverflow.com/questions/2100654/ignore-auto-repeat-in-x11-applications

#if defined(V_DEBUG)
      printf("xe.xkey.keycode: %d\n", xe.xkey.keycode);
#endif // V_DEBUG

      if (xe.xkey.keycode == w->keycode_dn) { // X_KEY_DN
        w->keycode_dn_pressed = 1;
        info->ascii_value = KEY_DOWN;
        XUngrabKey(foreground_dpy, w->keycode_dn, 0, *top_window);
        //XUngrabKeyboard(foreground_dpy, CurrentTime);
        return 0;
      } else if (xe.xkey.keycode == w->keycode_up) { // X_KEY_UP
        //XUngrabKeyboard(foreground_dpy, CurrentTime);
        info->ascii_value = KEY_UP;
        w->keycode_up_pressed = 1;
        XUngrabKey(foreground_dpy, w->keycode_up, 0, *top_window);
        return 0;
      } else if (xe.xkey.keycode == w->keycodes[0]) { // XK_End
        //XUngrabKeyboard(foreground_dpy, CurrentTime);
        XUngrabKey(foreground_dpy, w->keycodes[0], 0, *top_window);
        info->ascii_value = KEY_END;
        w->keycode_end_pressed = 1;
        return 0;
      } else if (xe.xkey.keycode == w->keycodes[1]) { // XK_Begin
        //XUngrabKeyboard(foreground_dpy, CurrentTime);
        XUngrabKey(foreground_dpy, w->keycodes[1], 0, *top_window);
        info->ascii_value = KEY_HOME;
        w->keycode_beg_pressed = 1;
        return 0;
      } else if (xe.xkey.keycode == w->keycodes[2]) { // XK_BackSpace
        //XUngrabKeyboard(foreground_dpy, CurrentTime);
        XUngrabKey(foreground_dpy, w->keycodes[2], 0, *top_window);
        info->ascii_value = KEY_BACKSPACE;
        w->keycode_bckspce_pressed = 1;
        return 0;
      } else if (xe.xkey.keycode == w->keycodes[3]) { // XK_Page_Down
        w->keycode_page_dn_pressed = 1;
        info->ascii_value = KEY_PAGE_DN;
        XUngrabKey(foreground_dpy, (long)XLookupKeysym(&xe.xkey, 0), 0, *top_window);
        //XUngrabKey(foreground_dpy, w->keycodes[3], 0, *top_window);
        //XUngrabKeyboard(foreground_dpy, CurrentTime);
        return 0;
      } else if (xe.xkey.keycode == w->keycodes[4]) { // XK_Page_Up
        w->keycode_page_up_pressed = 1;
        info->ascii_value = KEY_PAGE_UP;
        XUngrabKey(foreground_dpy, (long)XLookupKeysym(&xe.xkey, 0), 0, *top_window);
        //XUngrabKey(foreground_dpy, w->keycodes[4], 0, *top_window);
        //XUngrabKeyboard(foreground_dpy, CurrentTime);
        return 0;
      } else if (xe.xkey.keycode == w->keycodes[5]) {
        w->keycode_escape_pressed = 1;
        info->ascii_value = KEY_ESCAPE;
        return 0;
        //XUngrabKey(foreground_dpy, (long)XLookupKeysym(&xe.xkey, 0), 0, *top_window);
      } else if (xe.xkey.keycode == w->keycodes[6]) {
        w->keycode_copy_pressed = 1;
        info->ascii_value = 'c';
        image_cp_signal = 1;
        // quit x11 and come back to it
        copy(&file_to_be_copied, img->path, strlen(img->path));
        return 0;
        //XUngrabKey(foreground_dpy, (long)XLookupKeysym(&xe.xkey, 0), 0, *top_window);
      } else {
        *key = xe.xkey.keycode;
        XUngrabKey(foreground_dpy, (long)XLookupKeysym(&xe.xkey, 0), 0, *top_window);
        //XUngrabKeyboard(foreground_dpy, CurrentTime);
        return 0;
      }
    }
    default:
      return 1;
  }
  // set marks for files the same waay vim sets marks for line numbers
}

// https://opensource.apple.com/source/X11libs/X11libs-60/mesa/Mesa-7.8.2/src/glut/glx/glut_event.c.auto.html
// If we are ignoring auto repeated keys for this window,
// check if the next event in the X event queue is a KeyPress
// for the exact same key (and at the exact same time) as the
// key being released.  The X11 protocol will send auto
// repeated keys as such KeyRelease/KeyPress pairs. */
int check_if_key_press2(InfoKeyPresses *info)
{
  XEvent event = { 0 };
  XEvent ahead = { 0 };
  XNextEvent(foreground_dpy, &ahead);
  switch (ahead.type) {
    case KeyPress:
      if (XEventsQueued(foreground_dpy, QueuedAfterReading)) {
        XPeekEvent(foreground_dpy, &ahead);
        if (ahead.type == KeyPress
            && ahead.xkey.window == event.xkey.window
            && ahead.xkey.keycode == event.xkey.keycode
            && ahead.xkey.time == event.xkey.time) {
          // Pop off the repeated KeyPress and ignore
          //   the auto repeated KeyRelease/KeyPress pair.
          XNextEvent(foreground_dpy, &event);
          return 0;
          //break;
        }
      }
  }

  return 1;
}

void move_and_return(int vert, int horiz, int returnVert, int returnHoriz, char *_str)
{
  sprintf(position, place_, vert, horiz);
  move(__file_descriptor, position);
  write_line(__file_descriptor, _str);
  sprintf(position, place_, returnVert, returnHoriz);
  move(__file_descriptor, position);
}

unsigned long set_img(char *path, InfoKeyPresses *info, STAT_INFO *info_file)
{
#if defined(EBUG)
  signal(SIGSEGV, handlern);
#endif // EBUG
  XSetErrorHandler(x_error_handler);
  foreground_dpy = NULL;
  if ((foreground_dpy = XOpenDisplay(NULL)) == NULL) {
    fprintf(stderr, "Error XOpenDisplay.\n");
    exit(1);
  }

//  *
//  * 0 Image BEGIN
//  *
  Image img = { 0 };

  int y_px = 90;


  Window root = DefaultRootWindow(foreground_dpy);
  Window term_window = get_focus_window(foreground_dpy); // parent window id

  tmp_window = term_window;
  target_win = get_toplevel_parent(foreground_dpy, tmp_window);
#if defined(V_DEBUG_POSITION)
  printf("tmp_window = 0x%lx | target_win = 0x%lx\n", tmp_window, target_win);
#endif // V_DEBUG_POSITION

#if defined(V_DEBUG_POSITION)
  printf("term_window = 0x%lx\n", term_window);
#endif // V_DEBUG_POSITION
  term_window = get_top_window(foreground_dpy, term_window);
#if defined(V_DEBUG_POSITION)
  printf("term_window = 0x%lx\n", term_window);
#endif // V_DEBUG_POSITION
  setup_mapping();

  XWindowAttributes xwa_image;
  xwa_image.override_redirect = TRUE;
  XWindowAttributes xwa;
  if (!XGetWindowAttributes(foreground_dpy, target_win, &xwa)) {
    fprintf(stderr, "Error XGetWindowAttributes\n");
  }
  img.size = info_file->file_len;

//  *
//  * 1 Create window BEGIN
//  *
  Win win = {};
  win.keycode_beg_pressed = 0;
  win.keycode_end_pressed = 0;
  win.keycode_bckspce_pressed = 0;
  win.keycode_dn_pressed = 0;
  win.keycode_up_pressed = 0;
  win.background_win = term_window;
  create_window(&win, &root, 0, y_px, &img, path);


//  *
//  * 1 Create window END
//  *

/*
)
  if (target_win != tmp_window) {
    XSelectInput(foreground_dpy,target_win,KeyPressMask|PropertyChangeMask|StructureNotifyMask);
  }
  XSelectInput(foreground_dpy,target_win,KeyPressMask|PropertyChangeMask|StructureNotifyMask);
  XSelectInput(foreground_dpy,term_window,KeyPressMask|PropertyChangeMask|StructureNotifyMask);

*/

//  *
//  * 2 Grab Key BEGIN
//  *
/*


)
  grab_keys(&win);
*/
//  *
//  * 2 Grab Key END
//  *

//  *
//  * 3 Put Image BEGIN
//  *
  Atom active_window;

  Atom_Prop atom_prop = { 0 };
  Atom_Prop child_win = { 0 };

  int result_key_press = 0;
  put_image(&win, &img, 0, y_px);


  //if (check_if_key_press2(info, &tmp_window, &win) == 0) {
  if (img.height > 450 && img.width > 400) {
    if (check_if_key_press2(info) == 0) {
      if (result_key_press == 0) {
        goto finish;
      }
    }
  }

  img.ximage = XCreateImage(foreground_dpy, CopyFromParent, img.depth, ZPixmap,
                            0, (char *)img.data_resized, img.new_width, img.new_height,
                            img.bpl * 8, img.bpl * img.new_width);
  img.gc = XCreateGC(foreground_dpy, win.foreground_win, 0, 0);
  XFlush(foreground_dpy);
  XSync(foreground_dpy, False);
  nanosleep((const struct timespec[]){{0, 7000000L}}, NULL);
  for (;;) {
    XEvent event = { 0 };
    XNextEvent(foreground_dpy, &event);
    if (event.type == MapNotify) {
      break;
    }
  }
#if defined(V_DEBUG_POSITION)
  printf("img.ximage[0] = %d\n", img.ximage->data[0]);
#endif
  //nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);
  //nanosleep((const struct timespec[]){{0, 7000000L}}, NULL);
  nanosleep((const struct timespec[]){{0, 9000000L}}, NULL);
  XPutImage(foreground_dpy, win.foreground_win,
            img.gc, img.ximage,
            0, 0,
            0, 0,
            img.new_width, img.new_height);
  XFlush(foreground_dpy);
  //if (check_if_key_press2(info, &tmp_window, &win) == 0) {

  if (img.height > 450 && img.width > 400) {
    if (check_if_key_press2(info) == 0) {
      if (result_key_press == 0) {
        goto finish;
      }
    }
  }
  //nanosleep((const struct timespec[]){{0, 7000000L}}, NULL);
//  *
//  * 3 Put Image END
//  *

  int stop = 0;
  Atom wmDeleteMessage = XInternAtom(foreground_dpy, "WM_DELETE_WINDOW", False);
//  *
//  * 4 Set WM BEGIN
//  *
  XSetWMProtocols(foreground_dpy, win.foreground_win, &wmDeleteMessage, 1);
//  *
//  * 4 Set WM END
//  *

  XEvent event_foreground = { 0 };

  //int window_set_above = 0;
#if defined(V_DEBUG)
  //fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
  //printf("target_win = 0x%lx | tmp_window = 0x%lx | root = 0x%lx | foreground_win = 0x%lx\n",
  //    target_win, tmp_window, root, /*foreground_win */ win.foreground_win);
#endif

  elapsedTime = 0.0;
  pastElapsedTime = 0.0;
  KeyCode key = 0;

  counter_position = 0;
  y_pos_debug = 1;

  img.size = 0;
  copy(&(img.path), path, strlen(path));

  while (process_event3(&img.gc, &tmp_window, &wmDeleteMessage, &win, &atom_prop, &img, &key, info)) {

    //ttymode_reset(ECHO, 1);
    if (img.size > 0) {

    XSetInputFocus(foreground_dpy, win.background_win, RevertToPointerRoot, CurrentTime);
      if (!XGetWindowAttributes(foreground_dpy, term_window, &xwa)) {
        fprintf(stderr, "Error XGetWindowAttributes\n");
      }
      //sprintf(imgsize, IMGSIZE, img.size);
      //printf("%lu", img.size);
      //move_and_return(xwa.height - 2, 1, 1, 1, imgsize);
      //mvprint_goback(xwa.height - 1, 1, 1, 1, imgsize);
      //printTTY_UL(xwa.height - 1, 1, img.size);
    }
    img.size = 0;
    elapsedTime = 0;
    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
    pastElapsedTime = elapsedTime;
  }
  if (img.path != NULL) {
    free(img.path);
    img.path = NULL;
  }

  erase_size(1, xwa.height - 1);
/*
  while (!stop) {

    FD_ZERO(&in_fds);
    FD_SET(fd_foreground, &in_fds);
    FD_SET(fd_background, &in_fds);

    tv.tv_usec = 0;
    tv.tv_sec = 0;

    num_fds = select(fd_foreground + 1, &in_fds, NULL, NULL, &tv);

    if (num_fds > 0) {
      continue;
    } else if (num_fds != 0) {
      fprintf(stderr, "An error occured num_fds.\n");
    }

    while(XPending(foreground_dpy)) {
      XNextEvent(foreground_dpy, &event_foreground);
    }

///
    if (target_win != tmp_window) {
      XSelectInput(foreground_dpy, tmp_window,
                   KeyPressMask|PropertyChangeMask|StructureNotifyMask);
    } else {
      XSelectInput(foreground_dpy, target_win,
                   KeyPressMask|PropertyChangeMask|StructureNotifyMask);
    }
//

//    XSelectInput(foreground_dpy, term_window,
//        KeyPressMask|PropertyChangeMask|StructureNotifyMask);
//  /
//  / 5 Set Select Input BEGIN
//  /
      XSelectInput(foreground_dpy, target_win,
                   KeyPressMask|PropertyChangeMask|StructureNotifyMask);
    if (window_unmapped != 1) {
//      XSelectInput(foreground_dpy, win.foreground_win,
//                   KeyPressMask|PropertyChangeMask|StructureNotifyMask);
    } else {
      XSelectInput(foreground_dpy, target_win,
                   KeyPressMask|PropertyChangeMask|StructureNotifyMask);

//      XSelectInput(foreground_dpy, term_window,
//                   KeyPressMask|PropertyChangeMask|StructureNotifyMask);
    }
//  /
//  / 5 Set Select Input BEGIN
//  /

// ////////////
    //event_foreground.xkey.keycode = 0;
    XNextEvent(foreground_dpy, &event_foreground);

//    if (window_unmapped != 1) {

//      show_properties(&atom_prop, &term_window, 0);
////
      if (target_win != tmp_window) {
        //show_properties(&atom_prop, &tmp_window, 0);
        show_properties(&atom_prop, &tmp_window, 1);
      } else {
        show_properties(&atom_prop, &target_win, 1);
      }
////
//    }

//
//    if (!strstr(formatting_buffer, "HIDDEN") && window_unmapped != 1) {
//#if defined(V_DEBUG)
//      printf("formatting_buffer: %s\n", formatting_buffer);
//#endif
//      active_window = XInternAtom(foreground_dpy, "_NET_ACTIVE_WINDOW", True);
//    }
//#if defined(V_DEBUG)
//    printf("event_foreground.xkey.keycode = %u\n", event_foreground.xkey.keycode);
//#endif
//

//  /
//  / 6 Key Code Check BEGIN
//  /
//
//
    if (event_foreground.xkey.keycode == win.keycode_dn) {
      stop = 1;
      XUngrabKey(foreground_dpy, win.keycode_dn, 0, win.grab_window);
    } else if (event_foreground.xkey.keycode == win.keycode_up) {
      stop = 1;
      XUngrabKey(foreground_dpy, win.keycode_up, 0, win.grab_window);
    }
//  /
//  / 6 Key Code Check END
//  /

    if (target_win != tmp_window) {
#if defined(V_DEBUG)
      //fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
      //printf("target_win = 0x%lx | tmp_window = 0x%lx\n", target_win, tmp_window);
#endif
//      XSelectInput(foreground_dpy, tmp_window,
//                   KeyPressMask|PropertyChangeMask|StructureNotifyMask);
    } else {
#if defined(V_DEBUG)
      //fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
      //printf("target_win = 0x%lx | tmp_window = 0x%lx\n", target_win, tmp_window);
#endif
//      XSelectInput(foreground_dpy, target_win,
//                   KeyPressMask|PropertyChangeMask|StructureNotifyMask);
    }
    if (event_foreground.type == UnmapNotify) {
#if defined(V_DEBUG)
      fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
      printf("UnmapNotify\n");
      sleep(5);
#endif
    }
    if (event_foreground.type == ConfigureNotify) {

      if (strstr(atom_prop.status, "HIDDEN")) {
#if defined(V_DEBUG)
      fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
      printf("UnmapNotify\n");
      sleep(5);
#endif
        XUnmapWindow(foreground_dpy, win.foreground_win);
//        sleep(2);
        XFlush(foreground_dpy);
        //if (foreground_dpy && img.gc) {
        //  XFreeGC(foreground_dpy, img.gc);
        //}
        window_unmapped = 1;
      }
      if (!strstr(atom_prop.status, "HIDDEN")
          && window_unmapped == 1
          && strstr(atom_prop.status, "MAXIMIZED")) {
#if defined(V_DEBUG)
        printf("atom_prop.status: %s\n", atom_prop.status);
        printf("formatting_buffer: %s\n", formatting_buffer);
        printf("Map Window\n");
        sleep(5);
#endif
        XMapWindow(foreground_dpy, win.foreground_win);
        XFlush(foreground_dpy);
        //img.gc = XCreateGC(foreground_dpy, win.foreground_win, 0, 0);
        //printf("img.gc[0] = %d\n", img.gc);
        window_unmapped = 0;
      }

      if (window_unmapped == 0) {
        if (!XGetWindowAttributes(foreground_dpy, target_win, &xwa)) {
        //if (!XGetWindowAttributes(foreground_dpy, term_window, &xwa)) {
          fprintf(stderr, "Error XGetWindowAttributes\n");
        }
      }
//  /
//  / 7 WIN BEGIN
//  /
//
//      if (!XGetWindowAttributes(foreground_dpy, win.foreground_win, &xwa_image)) {
//        fprintf(stderr, "Error XGetWindowAttributes xwa_image\n");
//      }
//#if defined(V_DEBUG_POSITION)
//      printf("\txwa.x       = %d, xwa.y       = %d\n", xwa.x, xwa.y);
//      printf("\txwa.width   = %d, xwa.height  = %d\n", xwa.width, xwa.height);
//      printf("\txwa_image.x = %d, xwa_image.y = %d\n", xwa_image.x, xwa_image.y);
//#endif
///
//  *
//  * 7 WIN END
//  *


#if defined(V_DEBUG_POSITION)
      printf("\txwa.x       = %d, xwa.y       = %d\n", xwa.x, xwa.y);
      printf("\txwa.width   = %d, xwa.height  = %d\n", xwa.width, xwa.height);
#endif

//
//      if (window_unmapped != 1) {
//        XUnmapWindow(foreground_dpy, win.foreground_win);
//        window_unmapped = 1;
//      }
//      if (img.ximage) {
//        XFree(img.ximage);
//      }
//      if (foreground_dpy && img.gc) {
//        XFreeGC(foreground_dpy, img.gc);
//      }
//      if (foreground_dpy && win.foreground_win) {
//        XDestroyWindow(foreground_dpy, win.foreground_win);
//      }
//      if (img.data_resized) {
//        free(img.data_resized);
//        img.data_resized = NULL;
//      }
//
      if (xwa_image.x > xwa.x + xwa.width) {
        fprintf(stderr, "Error Image outside of background window width\n");
      }
      if (xwa_image.y > xwa.y + xwa.height) {
        fprintf(stderr, "Error Image outside of background window height\n");
      }

      if (xwa.x != x_pos || xwa.y != y_pos) {
        x_pos = xwa.x;
        y_pos = xwa.y;
        x_mv = xwa.x + x_px;
        y_mv = xwa.y + y_px;

        if (xwa_image.x > xwa.x + xwa.width) {
          fprintf(stderr, "Error Image outside of background window width\n");
        }
        if (xwa_image.y > xwa.y + xwa.height) {
          fprintf(stderr, "Error Image outside of background window height\n");
        }

        if (x_mv > xwa.x + xwa.width) {
          x_mv = xwa.x + xwa.width - xwa_image.width;
        }
        if (y_mv > xwa.y + xwa.height) {
          y_mv = xwa.y + xwa.height - xwa_image.height;
        }

//  /
//  / 7 WIN BEGIN
//  /
//
//        if (!XMoveWindow(foreground_dpy, win.foreground_win, x_mv, y_mv)) {
//          fprintf(stderr, "Error XMoveWindow foreground_win\n");
//        }
//
//  /
//  / 7 WIN END
//  /

      } else if (xwa.width != width_bg_win || xwa.height != height_bg_win) {
        width_bg_win = xwa.width;
        height_bg_win = xwa.height;
      }
    }
  }
*/
/*
  if (info->n_times_pressed > 1) {
    for (size_t i = 0; i < info->n_times_pressed; ++i) {
      //XUngrabKey(foreground_dpy, int, unsigned int, top_window);
      XUngrabKey(foreground_dpy, (long)info->keypress_value, 0, tmp_window);
    }
    info->n_times_pressed = 0;
  }
*/
  if (atom_prop.status) {
    free(atom_prop.status);
    atom_prop.status = NULL;
  }
  if (child_win.status) {
    free(child_win.status);
    child_win.status = NULL;
  }
//  *
//  * 8 WIN BEGIN
//  *
finish:
  if (img.data_resized) {
    free(img.data_resized);
    img.data_resized = NULL;
  }
  if (img.data) {
    stbi_image_free(img.data);
    img.data = NULL;
  }
  if (img.ximage) {
    XFree(img.ximage);
  }
  if (foreground_dpy && img.gc) {
    XFreeGC(foreground_dpy, img.gc);
  }
  if (foreground_dpy && win.foreground_win) {
    XDestroyWindow(foreground_dpy, win.foreground_win);
  }
//  *
//  * 8 WIN END
//  *
  close_display();
  if (property_formats) {
    free(property_formats);
    property_formats = NULL;
  }
  if (win.keycode_dn_pressed) {
    //return 0x006a;
    return KEY_DOWN;
  } else if (win.keycode_up_pressed) {
    //return 0x006b;
    return KEY_UP;
  } else if (win.keycode_end_pressed) {
    return KEY_END;
  } else if (win.keycode_beg_pressed) {
    //printf("KEY HOME PRESSED\n");
    return KEY_ALL_UP;
  } else if (win.keycode_bckspce_pressed) {
    return BACKSPACE;
  } else if (win.keycode_page_dn_pressed) {
    //printf("wait");
    //sleep(5);
    return KEY_PAGE_DN;
  } else if (win.keycode_page_up_pressed) {
    return KEY_PAGE_UP;
  } else if (win.keycode_escape_pressed) {
    return KEY_ESCAPE;
  } else if (win.keycode_copy_pressed) {
    return 'c';
  } else {
    return key;
  }
  return 0;
}
