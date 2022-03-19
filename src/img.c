#include "img.h"
#include "scr.h"
#include <X11/Xlib.h>
#include <unistd.h>
#define STBI_NO_HDR
#define STBI_NO_LINEAR
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
    fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
    printf("len_element %lu, atom_prop->total_len = %d\natom_prop->status[%d]: %s\n",
        len_element, atom_prop->total_len, atom_prop->total_len, atom_prop->status);
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

Window select_args(int *rargc, char **argv)
{
  Window w = 0;
  int argc = *rargc;
#define OPTION argv[0]
#define NXTOPTP ++argv, --argc > 0
#define NXTOPT(arg) if (++argv, --argc == 0) printf("%s requires an argument\n", (arg))
  while (NXTOPTP) {
    if (!strcmp(OPTION, "-id")) {
      NXTOPT("-id");
      w = 0;
      sscanf(OPTION, "0x%lx", &w);
      if (!w)
        sscanf(OPTION, "%lu", &w);
      if (!w)
        error("Invalid window id format: %s.", OPTION);
      //continue;
      break;
    }
  }
  return w;
}

Window get_child_window(Display *display, Window window)
{
  Window root;
  Window parent;
  Window *children = NULL;
  unsigned int num_children;
  Window window_copy;

  for (;;) {
    if (!XQueryTree(display, window, &root, &parent, &children, &num_children)) {
      fprintf(stderr, "XQueryTree error: %s\n", __func__);
    }
    if (window == root || parent == root) {
      window_copy = *children;
      if (children) {
        XFree(children);
      }
      return window_copy;
    }
    else {
      window = parent;
    }
  }
  return -1;
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
  //if (img->width > (*width / 2)) {
  if (*image_width > (*width / 2)) {
    //fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
    //printf("width / 2 = %d, img->width = %d\n", width / 2, img->width);
    temp_width = (double)(*width / 2);
    //factor = temp_width / img->width;
    factor = temp_width / *image_width;
    //*width *= factor;
    *image_width *= factor;
    *width_taken = 1;
  }
  //else if ((double)(img->height) > (double)(*height - upper_lower_limit)) {
  else if ((double)(*image_height) > (double)(*height - upper_lower_limit)) {
    //temp_height = (double)(height - 100.0);
    //factor = temp_height / temp_height;
    //factor = temp_height / (double)img->height;
    factor = temp_height / (double)(*image_height);
    //*height *= factor;
    *image_height *= factor;
  }

  if (*image_width > (*width / 2)) {
    factor = 1 / (*image_width / (*width / 2));
  printf("factor = %f  ", factor);

  }

/*
  if (*width_taken) {
    //if ((double)(img->height) > (double)(*height - upper_lower_limit)) {
    if ((double)(*image_height) > (double)(*height - upper_lower_limit)) {
      temp_height = (double)(*height - upper_lower_limit);
      //factor = temp_height / (double)img->height;
      factor = temp_height / (double)(*image_height);
      // *height *= factor;
      *image_height *= factor;
    }
  }
  */

  return factor;
}

double fix_factor_to_fit_inside_window(Image *img, int width, int height)
{
  double factor = 1.0;
  //double new_width = width;
  //double new_height = height;
  //double temp_width;
  double upper_lower_limit = 190.0;
  //double temp_height = (double)((double)height - upper_lower_limit);
  //int width_taken = 0;
  //int new_image_width = img->width;
  //int new_image_height = img->height;

  if (img->width > (width / 2) - 50) {
    factor = 1.0 / (((double)img->width) / ((double)((width / 2) - 50)));
    //printf("factor = %f  ", factor);
    //if (factor * img->height < temp_height) {
      //return factor;
    //}
    //else {
      //temp_width = (double)(width / 2);
      //factor = temp_width / img->width;
      //double value = temp_width / img->width;
      //factor = (value < factor) ? value : factor;
      //*width *= factor;
    //}
  }
  //double value = temp_height / img->height;
  double value = (double)((double)height - upper_lower_limit) / img->height;
  return (value < factor) ? value : factor;
/*
  else {
      temp_width = (double)(width / 2);
      factor = temp_width / img->width;
      // *width *= factor;
    }
*/

// new_width / 2 = 950 px
  //while (img->width > (new_width / 2) || img->height > new_height - upper_lower_limit) {
/*
  while (new_image_width > (new_width / 2) || new_image_height > new_height - upper_lower_limit) {
    //factor = loop_fix(img, width, height, temp_width, temp_height, &width_taken, factor, upper_lower_limit);
    //factor = loop_fix(img, &new_width, &new_height, temp_width, temp_height, &width_taken, factor, upper_lower_limit);
    factor = loop_fix(img, &new_width, &new_height, temp_width, temp_height, &width_taken, factor, upper_lower_limit, &new_image_width, &new_image_height);

    //printf("width / 2 = %d, new_image_width = %d\n", width / 2, new_image_width);
  }
*/
  //factor = new_width /
  //return factor;
}

void create_window(Win *win, Window *root, int x_px, int y_px, Image *img, char *path)
{
  int width = DisplayWidth(foreground_dpy, win->screen);
  int height = DisplayHeight(foreground_dpy, win->screen);
  // Load image into data variable
  //img.data = stbi_load(argv[3], &(img.width), &(img.height), &(img.n), 4);
  img->data = stbi_load(path, &(img->width), &(img->height), &(img->n), 4);

  //double factor = atof(argv[4]);
  //double factor = factor_in;
  double factor = 1.0;
/*
  double temp_width;
  double temp_height;
  if (img->width > (width / 2)) {
    fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
    //printf("width / 2 = %d, img->width = %d\n", width / 2, img->width);
    temp_width = (double)(width / 2);
    factor = temp_width / img->width;
  } else if ((double)(img->height) > (double)((double)height - 100.0)) {
    temp_height = (double)(height - 100.0);
    factor = temp_height / temp_height;
  }
*/
  factor = fix_factor_to_fit_inside_window(img, width, height);
  //factor = 0.5;
  //printf("factor = %f  ", factor);
  //img->new_width = (double)img->width * factor;
  //img->new_height = (double)img->height * factor;
  img->new_width = img->width * factor;
  img->new_height = img->height * factor;
  //fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
  //printf("img _ width = %d, height = %d  ", img->width, img->height);
  //printf("new_width = %f, new_height = %f\n", img->new_width, img->new_height);
  //printf("new_width = %d, new_height = %d\n", img->new_width, img->new_height);

  // Allocate Memory for data_resized variable with new_width & new_height
  //img->data_resized = malloc((img->new_width * img->new_height * 4) * sizeof *img->data_resized);
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

  win->screen = DefaultScreen(foreground_dpy);
#if defined(V_DEBUG_POSITION)
  printf("win->screen = %d\n\t", win->screen);
  printf("x_px = %d | y_px = %d\n\t", x_px, y_px);
#endif
  //win->foreground_win = XCreateSimpleWindow(foreground_dpy, *root, x_px, y_px,
  int center_in_second_window_dist = width / 4;
  win->foreground_win = XCreateSimpleWindow(foreground_dpy, *root, (width / 2) + center_in_second_window_dist - img->new_width / 2, 49,
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
}

void grab_keys(Win *win)
{
  unsigned int   modifiers       = 0;
  int            owner_events    = 0;
  int            pointer_mode    = GrabModeAsync;
  int            keyboard_mode   = GrabModeAsync;

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
/*
  //XGrabKey(foreground_dpy, win->keycodes[0], Mod2Mask,
  //         win->grab_window, owner_events, pointer_mode,
  //         keyboard_mode);
  XGrabKey(foreground_dpy, win->keycodes[1], modifiers, // XK_Begin
           win->grab_window, owner_events, pointer_mode,
           keyboard_mode);
  //XGrabKey(foreground_dpy, win->keycodes[1], Mod2Mask,
  //         win->grab_window, owner_events, pointer_mode,
  //         keyboard_mode);
  XGrabKey(foreground_dpy, win->keycodes[2], modifiers, // XK_BackSpace
           win->grab_window, owner_events, pointer_mode,
           keyboard_mode);
  //XGrabKey(foreground_dpy, win->keycodes[2], Mod2Mask,
  //         win->grab_window, owner_events, pointer_mode,
  //         keyboard_mode);
*/
}

//void put_image(Win *win, Image *img)
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
  nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);

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

  //printf("getting top window ... \n");
  while (parent != root) {
    w = parent;
    s = XQueryTree(d, w, &root, &parent, &children, &nchildren); // see man
    if (s) {
      XFree(children);
    }
    if (xerror) {
      printf("fail\n");
      exit(1);
    }
    //printf("  get parent (window: 0x%lx)\n", w);
  }
  //printf("success (window: 0x%lx)\n", w);
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

int process_event(GC *gc,
                  Window *top_window,
                  Atom *wmDeleteMessage,
                  Win *w,
                  Atom_Prop *atom_prop, Image *img)
{
  XEvent xe;
  XNextEvent(foreground_dpy, &xe);
  XSelectInput(foreground_dpy, *top_window, KeyPressMask | ExposureMask| PropertyChangeMask | StructureNotifyMask);

  show_properties(atom_prop, &tmp_window, 1);
  if (strstr(atom_prop->status, "HIDDEN")) {
#if defined(V_DEBUG)
    fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
    printf("\t\t\t\t\t\t\tHIDDEN\n\n\n\n\n\n");
#endif
    XUnmapWindow(foreground_dpy, w->foreground_win);
    XFlush(foreground_dpy);
    window_unmapped = 1;
    window_remapped = 0;
  } else if (window_unmapped == 1) {
    XMapWindow(foreground_dpy, w->foreground_win);
    XSync(foreground_dpy, False);
    nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);
    XFlush(foreground_dpy);
    //XSync(foreground_dpy, False);
#if defined(V_DEBUG_POSITION)
    printf("img.ximage[0] = %d\n", img.ximage->data[0]);
#endif
    nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);
    XPutImage(foreground_dpy, w->foreground_win,
              *gc, img->ximage,
              0, 0,
              0, 0,
              img->new_width, img->new_height);
    XFlush(foreground_dpy);
    XSync(foreground_dpy, False);
    window_remapped = 1;
    window_unmapped = 0;
  }
  if (atom_prop->status) {
    free(atom_prop->status);
    atom_prop->status = NULL;
  }
  switch (xe.type) {
    case MapNotify:
      XSync(foreground_dpy, False);
      return 1;
      break;
    case ClientMessage:
      if (xe.xclient.message_type == *wmDeleteMessage) {
        return 0;
      }
    case KeyPress:
#if defined(V_DEBUG)
      printf("xe.xkey.keycode: %d\n", xe.xkey.keycode);
#endif // V_DEBUG
      if (xe.xkey.keycode == w->keycode_dn) { // X_KEY_DN
        w->keycode_dn_pressed = 1;
        //XUngrabKey(foreground_dpy, w->keycode_dn, 0, *top_window);
        return 0;
      } else if (xe.xkey.keycode == w->keycode_up) { // X_KEY_UP
        //XUngrabKey(foreground_dpy, w->keycode_up, 0, *top_window);
        w->keycode_up_pressed = 1;
        return 0;
      } else if (xe.xkey.keycode == w->keycodes[0]) { // XK_End
        //XUngrabKey(foreground_dpy, w->keycodes[0], 0, *top_window);
        w->keycode_end_pressed = 1;
        return 0;
      } else if (xe.xkey.keycode == w->keycodes[1]) { // XK_Begin
        //XUngrabKey(foreground_dpy, w->keycodes[1], 0, *top_window);
        w->keycode_end_pressed = 1;
        return 0;
      } else if (xe.xkey.keycode == w->keycodes[2]) { // XK_BackSpace
        //XUngrabKey(foreground_dpy, w->keycodes[2], 0, *top_window);
        w->keycode_bckspce_pressed = 1;
        return 0;
      }
      else {
        XUngrabKey(foreground_dpy, xe.xkey.keycode, 0, *top_window);
        return 1;
      }
    default:
      return 1;
  }
  // set marks for files the same waay vim sets marks for line numbers
}

//int set_img(int argc, char **argv)
int set_img(__attribute__((__unused__)) int argc,
            __attribute__((__unused__)) char *prog_name,
            __attribute__((__unused__)) Window window_in,
            char *path,
            double factor_in,
            int y_pos_in, int x_pos_in)
{
#if defined(EBUG)
  signal(SIGSEGV, handlern);
#endif // EBUG
/*
  if (argc < 6) {
    fprintf(stderr, "Usage: %s -id 0x<window id> <path/to/image>"\
        " <factor> <y_pos>(px) <x_pos>(px)\n", prog_name);
    exit(1);
  }
*/
  foreground_dpy = NULL;
  if ((foreground_dpy = XOpenDisplay(NULL)) == NULL) {
    fprintf(stderr, "Error XOpenDisplay.\n");
    exit(1);
  }

//  *
//  * 0 Image BEGIN
//  *
  Image img = {};

  //int y_px = atoi(argv[6]);
  //int x_px = atoi(argv[5]);
  //int y_px = y_pos_in;
  int y_px = 90;
  int x_px = (x_pos_in / 2) + 10;

/*
  // Load image into data variable
  //img.data = stbi_load(argv[3], &(img.width), &(img.height), &(img.n), 4);
  img.data = stbi_load(path, &(img.width), &(img.height), &(img.n), 4);

  //double factor = atof(argv[4]);
  double factor = factor_in;
  img.new_width = img.width * factor;
  img.new_height = img.height * factor;

  // Allocate Memory for data_resized variable with new_width & new_height
  img.data_resized = malloc((img.new_width * img.new_height * 4) * sizeof *img.data_resized);
  if (img.data_resized == NULL) {
    fprintf(stderr, "Error malloc data_resized.\n");
    exit(1);
  }

  // Resize image from data to data_resized
  stbir_resize_uint8(img.data, img.width, img.height,
                     0, img.data_resized, img.new_width, img.new_height, 0, 4);

  int i = 0,
      len = img.new_width * img.new_height;
  unsigned int *dp = (unsigned int *)img.data_resized;
  for (; i < len; ++i) {
    dp[i] = (dp[i] & 0xFF00FF00) | ((dp[i] >> 16) & 0xFF) | ((dp[i] << 16) & 0xFF0000);
  }
*/
//  *
//  * 0 Image END
//  *
  Window root = DefaultRootWindow(foreground_dpy);
  Window term_window = get_focus_window(foreground_dpy); // parent window id
  //nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);

  //tmp_window = select_args(&argc, argv);
  //tmp_window = window_in;
  tmp_window = term_window;
  target_win = get_toplevel_parent(foreground_dpy, tmp_window);
#if defined(V_DEBUG_POSITION)
  printf("tmp_window = 0x%lx | target_win = 0x%lx\n", tmp_window, target_win);
#endif // V_DEBUG_POSITION

  //Window top_window; // term_window
  //Window term_window = get_focus_window(foreground_dpy); // parent window id
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
  //if (!XGetWindowAttributes(foreground_dpy, term_window, &xwa)) {
    fprintf(stderr, "Error XGetWindowAttributes\n");
  }

//  *
//  * 1 Create window BEGIN
//  *
  Win win = {};
  win.keycode_beg_pressed = 0;
  win.keycode_end_pressed = 0;
  win.keycode_bckspce_pressed = 0;
  win.keycode_dn_pressed = 0;
  win.keycode_up_pressed = 0;
  //create_window(&win, &root, x_px, y_px, &img);
  create_window(&win, &root, x_px, y_px, &img, path);
  //nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);
  //sleep(1);

//  *
//  * 1 Create window END
//  *

  if (target_win != tmp_window) {
    XSelectInput(foreground_dpy,target_win,KeyPressMask|PropertyChangeMask|StructureNotifyMask);
  }
  XSelectInput(foreground_dpy,target_win,KeyPressMask|PropertyChangeMask|StructureNotifyMask);

//  XSelectInput(foreground_dpy,term_window,KeyPressMask|PropertyChangeMask|StructureNotifyMask);
  //XSelectInput(foreground_dpy,tmp_window,KeyPressMask|PropertyChangeMask|StructureNotifyMask | ExposureMask);
  XSelectInput(foreground_dpy,term_window,KeyPressMask|PropertyChangeMask|StructureNotifyMask);


//  *
//  * 2 Grab Key BEGIN
//  *
  grab_keys(&win);
//  *
//  * 2 Grab Key END
//  *


//  *
//  * 3 Put Image BEGIN
//  *
  //put_image(&win, &img);
  put_image(&win, &img, x_px, y_px);
  img.ximage = XCreateImage(foreground_dpy, CopyFromParent, img.depth, ZPixmap,
                                0, (char *)img.data_resized, img.new_width, img.new_height,
                                img.bpl * 8, img.bpl * img.new_width);
  //nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);
  img.gc = XCreateGC(foreground_dpy, win.foreground_win, 0, 0);
  //nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);
  XFlush(foreground_dpy);
  XSync(foreground_dpy, False);
  //nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);
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
  nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);
  XPutImage(foreground_dpy, win.foreground_win,
            img.gc, img.ximage,
            0, 0,
            0, 0,
            img.new_width, img.new_height);
  XFlush(foreground_dpy);
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

/*
  fd_set in_fds;
  struct timeval tv;

  int fd_background = 0;
  int fd_foreground = 0;
  int num_fds       = 0;
  fd_foreground     = ConnectionNumber(foreground_dpy);

  int x_pos         = xwa.x;
  int y_pos         = xwa.y;
  int width_bg_win  = xwa.width;
  int height_bg_win = xwa.height;

  int x_mv            = 0;
  int y_mv            = 0;
  int window_unmapped = 0;
*/
  Atom active_window;

  Atom_Prop atom_prop = { 0 };
  Atom_Prop child_win = { 0 };

  int window_maximized = 0;

  int window_set_above = 0;
#if defined(V_DEBUG)
  //fprintf(stdout, "%s:%s:%d\n\t", __FILE__, __func__, __LINE__);
  //printf("target_win = 0x%lx | tmp_window = 0x%lx | root = 0x%lx | foreground_win = 0x%lx\n",
  //    target_win, tmp_window, root, /*foreground_win */ win.foreground_win);
#endif

  while (process_event(&img.gc, &tmp_window, &wmDeleteMessage, &win, &atom_prop, &img));
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
    return 0x006a;
  } else if (win.keycode_up_pressed) {
    return 0x006b;
  } else if (win.keycode_end_pressed) {
    return KEY_END;
  } else if (win.keycode_beg_pressed) {
    return KEY_HOME;
  } else if (win.keycode_bckspce_pressed) {
    return KEY_BACKSPACE;
    //return 'h';
  }
  //return event_foreground.xkey.keycode;
  return 0;
}
