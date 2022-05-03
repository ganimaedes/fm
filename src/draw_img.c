// gcc -Wall -DV_DEBUG draw_img.c -o draw_img -lX11 && ./draw_img
// https://www.linuxquestions.org/questions/programming-9/how-to-draw-color-images-with-xlib-339366/
// https://stackoverflow.com/questions/36119241/x11-ximage-manipulation/36138997#36138997
// libaosd/libaosd/aosd.c
//#include "toon.h"
//#include "mem.h"
// http://www.makelinux.net/alp/035.htm SHMGET if prev is img rewrite on it
// gcc -Wall -ggdb3 -O0 draw_img.c -o draw_img -lX11 -lm -pthread && valgrind --tool=helgrind ./draw_img
// gcc -Wall -ggdb3 -O0 -DWITH_STBI draw_img.c -o draw_img -lX11 -lm -pthread && valgrind --tool=helgrind ./draw_img ""
#include "draw_img.h"

#if defined(WITH_STBI)
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
#endif // WITH_STBI

Bool xerror = FALSE;

volatile sig_atomic_t raised = 0;
volatile sig_atomic_t just_unraised = 0;

volatile sig_atomic_t mapped = 0;
volatile sig_atomic_t just_unmapped = 0;

volatile sig_atomic_t config_check_event_mapped = 1;
volatile sig_atomic_t focusin_check_event_mapped = 1;
volatile sig_atomic_t focusout_check_event_mapped = 1;

volatile sig_atomic_t second_loop = 0;

threadInfo_t threads[] = { { PTHREAD_MUTEX_INITIALIZER, { NULL, '\0' }, "q" } };
static Image *_image;
static unsigned int total_len = 0;
pthread_t keypress_thread_id, memory_thread_id;
static char *image_path = NULL;

// libaosd/libaosd/aosd-internal.c
void set_window_properties(Display* dsp, Window win)
{
  // we're almost a _NET_WM_WINDOW_TYPE_SPLASH, but we don't want
  // to be centered on the screen.  instead, manually request the
  // behavior we want.

  // turn off window decorations.
  // we could pull this in from a motif header, but it's easier to
  // use this snippet i found on a mailing list.
  Atom mwm_hints = XInternAtom(dsp, "_MOTIF_WM_HINTS", False);
  struct _mwm_hints_setting {
    long flags, functions, decorations, input_mode;
  } mwm_hints_setting = { (1L << 1), 0L, 0L, 0L };

  XChangeProperty(dsp, win, mwm_hints, mwm_hints, 32,
                  PropModeReplace, (unsigned char *)&mwm_hints_setting, 4);

  Atom win_type = XInternAtom(dsp, "_NET_WM_WINDOW_TYPE", False);
  Atom win_type_notif[] = {
    XInternAtom(dsp, "_NET_WM_WINDOW_TYPE_NOTIFICATION", False),
  };
  XChangeProperty(dsp, win, win_type, XA_ATOM, 32,
                  PropModeReplace, (unsigned char *)&win_type_notif, 1);

  // always on top, not in taskbar or pager.
  Atom win_state = XInternAtom(dsp, "_NET_WM_STATE", False);
  Atom win_state_setting[] =
  {
    //XInternAtom(dsp, "_NET_WM_STATE_ABOVE", False),
    XInternAtom(dsp, "_NET_WM_STATE_BELOW", False),
    XInternAtom(dsp, "_NET_WM_STATE_STICKY", False),
    XInternAtom(dsp, "_NET_WM_STATE_SKIP_TASKBAR", False),
    XInternAtom(dsp, "_NET_WM_STATE_SKIP_PAGER", False)
  };
  XChangeProperty(dsp, win, win_state, XA_ATOM, 32,
                  PropModeReplace, (unsigned char *)&win_state_setting, 3);
}

// https://stackoverflow.com/questions/31535560/xlib-and-firefox-behavior
void map_notify_handler(XEvent local_event, Display* display)
{
#if defined(V_DEBUG)
  __PRINTDEBUG;
  write_line_debug(__file_descriptor, "----------Map Notify\n");
#endif
  Window trans = None;
  XGetTransientForHint(display, local_event.xmap.window, &trans);
  if(trans != None){
    XSetInputFocus(display, trans, RevertToParent, CurrentTime);
    //XSetInputFocus(display, trans, RevertToNone, CurrentTime);
  }
}

// https://stackoverflow.com/questions/31535560/xlib-and-firefox-behavior
// https://github.com/leahneukirchen/cwm/blob/linux/xevents.c
void check_event(Display *display, XEvent *local_event, Window *img_window, Atom_Prop *atom_prop)
{
  switch(local_event->type){
    case ConfigureNotify:
      if (mapped == 0 && just_unmapped == 1) {
        XMapRaised(display, *img_window);
        mapped = 1;
        just_unmapped = 0;
        config_check_event_mapped = 1;
      }
#if defined(V_DEBUG)
      __PRINTDEBUG; __PRINTMAPINFO("ConfigureNotify"); __PRINTEVENTSELECTIONS;
#endif
      break;
    case MotionNotify:
#if defined(V_DEBUG)
      __PRINTDEBUG;
      printf("Motion Notify\n");
#endif
      //  motion_handler(local_event, display);
      break;
    case CreateNotify:
      break;
    case MapNotify:
      // here MapNotify applies to term_window so we map the img_window as well
      XMapRaised(display, *img_window);
#if defined(V_DEBUG)
      __PRINTDEBUG; __PRINTMAPINFO("MapNotify");
#endif
      map_notify_handler(*local_event, display);
      break;
    case UnmapNotify:
      // here UnmapNotify applies to term_window so we unmap the img_window as well
      XUnmapWindow(display, *img_window);
#if defined(V_DEBUG)
      __PRINTDEBUG; __PRINTMAPINFO("UnMapNotify");
#endif
      break;
    case DestroyNotify:
#if defined(V_DEBUG)
      printf("Destroy Event\n");
#endif
      break;
    case ButtonPress:
#if defined(V_DEBUG)
      printf("Event button pressed\n");
#endif
      break;
    case KeyPress:
#if defined(V_DEBUG)
      printf("Keyboard key pressed\n");
#endif
      break;
    case FocusIn:
      if (mapped == 0 || just_unmapped == 1) {
        XMapRaised(display, *img_window);
        mapped = 1;
        just_unmapped = 0;
      }
      __PRINTDEBUG;
      //__PRINTMAPINFO("FocusIn");
      __PRINTMAPINFO2("FocusIn");
      break;
    case FocusOut:
      if (mapped == 1 && just_unmapped == 0) {
        XUnmapWindow(display, *img_window);
/*
        if (strstr(atom_prop->status, "HIDDEN")) {
          XLowerWindow(display, *img_window);
        }
*/
        mapped = 0;
        just_unmapped = 1;
      }
      __PRINTDEBUG;
      __PRINTMAPINFO2("FocusOut");
      break;
    default:
      __PRINTDEBUG;
      __PRINTMAPINFO2("default");
      break;
  }
}
/*
void ttymode_reset(int mode, int imode)
{
  int fd = 1;
  struct termios ioval;
  tcgetattr(STDIN_FILENO, &ioval);
  ((ioval).c_lflag) &= ~mode;


  while (tcsetattr(fd, TCSANOW, &ioval) == -1) {
    if (errno == EINTR || errno == EAGAIN) {
      continue;
    }
    write_line_debug(__file_descriptor, "Error occured while reset : errno = ");
    //write_line_debug(__file_descriptor, );
  }
}
*/

int processEvent(Display *display,
                 Window *term_window,
                 Window *img_window,
                 XImage *ximage,
                 int width,
                 int height,
                 Atom WM_message[2],
                 int *nth_call_to_process_event_function, Atom_Prop *atom_prop)
{
  XEvent ev;
  XLockDisplay(display);
// https://stackoverflow.com/questions/12871071/x11-how-to-delay-repainting-until-all-events-are-processed
  //Show_All_Props2(atom_prop, term_window, 1);
  if (XNextEvent(display, &ev) >= 0) {

    XSelectInput(display, *term_window, ExposureMask | PropertyChangeMask | StructureNotifyMask | FocusChangeMask | LeaveWindowMask);
    // https://stackoverflow.com/questions/3806872/window-position-in-xlib
    int x = 0, y = 0;
    XWindowAttributes xwa;
    Window child;
    XTranslateCoordinates(display, *term_window, *img_window, 0, 0, &x, &y, &child);
    XGetWindowAttributes(display, *img_window, &xwa);
    //printf("%d %d\n", x - xwa.x, y - xwa.y);
    check_event(display, &ev, img_window, atom_prop);
    switch (ev.type) {
      case Expose:
        XPutImage(display, *img_window, DefaultGC(display, 0), ximage, 0, 0, 0, 0, width, height);
        ++*nth_call_to_process_event_function;
        XUnlockDisplay(display);
        return 1;
      case ClientMessage:
        if (ev.xclient.message_type == WM_message[0]) {
          if (ev.xclient.data.l[0] == WM_message[1]) {
            XUnlockDisplay(display);
            return 0;
          }
        }
      case ConfigureNotify:
        __PRINTDEBUG;
        ++*nth_call_to_process_event_function;
        if (mapped == 1) {
          XUnmapWindow(display, *img_window);
          mapped = 0;
          just_unmapped = 1;
          __PRINTMAPINFO_N2("ConfigureNotify");
          //just_unraised = 1;
        } else if (mapped == 0) {
          XMapRaised(display, *img_window);
          mapped = 1;
          just_unmapped = 0;
          __PRINTMAPINFO_N2("ConfigureNotify");
        }
        XUnlockDisplay(display);
        return 1;
        break;
      default:
        if (second_loop == 1) {
          if (mapped == 0 || just_unmapped == 1) {
            XMapRaised(display, *img_window);
            //raised = 1;
            just_unmapped = 0;
            mapped = 1;
            second_loop = 0;
            ++*nth_call_to_process_event_function;
            __PRINTDEBUG;
            __PRINTMAPINFO2("default");
            XUnlockDisplay(display);
            return 1;
          } else if (mapped == 1 && just_unmapped == 0) {
            __PRINTDEBUG;
            write_line_debug(__file_descriptor, "ISMAPPED\n");
          }
        }
        XUnlockDisplay(display);
        return 1;
    }
  }
}

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
      write_line_debug(STDERR_FILENO, "fail\n");
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
    write_line_debug(STDERR_FILENO, "fail\n");
    exit(1);
  } else if (w == None) {
    write_line_debug(STDERR_FILENO, "no focus window\n");
    exit(1);
  } else {
    return w;
  }
  return w;
}

Window get_toplevel_parent(Display * display, Window window)
{
  Window parent;
  Window root;
  Window *children;
  unsigned int num_children;

  for (;;) {
    if (!XQueryTree(display, window, &root, &parent, &children, &num_children))
      write_line_debug(STDERR_FILENO, "XQueryTree error");
    if (children)
      XFree(children);
    if (window == root || parent == root)
      return window;
    else
      window = parent;
  }
}

// if 'q' is pressed quit the memory_thread ie stop allocating memory and free allocated memory
void *detect_keypress(void *arg)
{
  char key;
  //threadInfo_t *t = &threads[(int)arg];    // get pointer to thread
  threadInfo_t *t = &threads[0];    // get pointer to thread
  for (;;) {
    pthread_mutex_lock(&t->kqmutex);     // lock other threads out while we tamper
    key = '\0';                          // initialize key to NULL
    if (t->kqhead.next != NULL) {        // Anything queued up for us?
      pthread_cancel(memory_thread_id);
      keyQueue_t *kq = t->kqhead.next; // if so get ptr to key pkt
      key = kq->key;                   // fetch key from pkt
      t->kqhead.next = kq->next;       // Point to next key in queue (or NULL if no more queued up).
      free(kq);
    }
    pthread_mutex_unlock(&t->kqmutex);   // unlock key queue
    if (key != '\0') {                   // if we got a key, log it
      //write_line("pulled key "); write_line(&key); write_line(" from queue\n");
    }
    //usleep(1000);
    usleep(100);
  }
  pthread_exit(NULL);
}

/*
char find_file_type2(STAT_INFO *info)
{
  info->file = fopen(info->file_name, "rb");
  if (info->file == NULL) {
    fprintf(stderr, "Error opening file: %s\n", info->file_name);
    exit(1);
  }

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
  __PRINTDEBUG; printf("file type: %c\n", type);
quit_file_type:
  fseek(info->file, 0, SEEK_SET);
  fclose(info->file);
  return type;
}
*/

void closex(void *arg)
{
#if defined(EBUG)
  write_line_debug(__file_descriptor, __func__);
#endif // EBUG
  if (_image->ximage != NULL) {
    XDestroyImage(_image->ximage);
  }
  if (img_window != 0) {
    XDestroyWindow(display, img_window);
  }
  if (display != NULL) {
    XCloseDisplay(display);
  }
#if defined(WITH_STBI)
  if (_image->data) {
    stbi_image_free(_image->data);
    _image->data = NULL;
  }
  if (_image->data_resized) {
    free(_image->data_resized);
    _image->data_resized = NULL;
  }
  if (image_path) {
    free(image_path);
    image_path = NULL;
  }
#endif // WITH_STBI
  if (_image) {
    free(_image);
    _image = NULL;
  }
  pthread_exit(NULL);
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

#if defined(WITH_STBI)
int load_stbi()
{
  _image->data = stbi_load(image_path, &(_image->width), &(_image->height), &(_image->n), 4);
  if (_image->data == NULL) {
    PRINTWRITE("Error img->data\n");
    exit(1);
  }
  double factor = 1.0;

  factor = fix_factor_to_fit_inside_window(_image, xwa.width, xwa.height);
  _image->new_width = _image->width * factor;
  _image->new_height = _image->height * factor;

  // Allocate Memory for data_resized variable with new_width & new_height
  int new_width = (int)_image->new_width;
  int new_height = (int)_image->new_height;
  _image->data_resized = malloc((new_width * new_height * 4) * sizeof *_image->data_resized);
  if (_image->data_resized == NULL) {
    PRINTWRITE("Error malloc data_resized.\n");
    exit(1);
  }

  // Resize image from data to data_resized
  stbir_resize_uint8(_image->data, _image->width, _image->height,
                     0, _image->data_resized, _image->new_width, _image->new_height, 0, 4);
  int i = 0,
      len = _image->new_width * _image->new_height;
  unsigned int *dp = (unsigned int *)_image->data_resized;
  for (; i < len; ++i) {
    dp[i] = (dp[i] & 0xFF00FF00) | ((dp[i] >> 16) & 0xFF) | ((dp[i] << 16) & 0xFF0000);
  }
  return 1;
}

void put_image(Window *img_window, int x_px, int y_px)
{
  // https://stackoverflow.com/questions/11069666/cannot-get-xcreatesimplewindow-to-open-window-at-the-right-position
  XSizeHints    my_hints = {0};
  my_hints.flags  = PPosition | PSize;     // specify position and size
  my_hints.x      = x_px;                  // origin and size coords I want
  my_hints.y      = y_px;
  my_hints.width  = _image->new_width;
  my_hints.height = _image->new_height;
  XSetNormalHints(display, img_window, &my_hints);  // Where new_window is the new window

  _image->bpl = 0;
  nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);

  _image->depth = DefaultDepth(display, DefaultScreen(display));
  switch (_image->depth) {
    case 24:          _image->bpl = 4; break;
    case 16: case 15: _image->bpl = 2; break;
    default:          _image->bpl = 1; break;
  }
#if defined(V_DEBUG_POSITION)
  __PRINTDEBUG; PRINTVALUE(_image->depth);
#endif
}

#else
XImage *CreateTrueColorImage(Display *display,
                             Visual *visual,
                             unsigned char *image,
                             int width,
                             int height)
{
  int i, j;
  unsigned char *image32 = NULL;
  int len = width * height * 4;
  if (!(image32 = malloc(len * sizeof *image32))) {
    write_line_debug(STDERR_FILENO, "Error: malloc\n");
    exit (1);
  }
  memset(image32, 0, len * sizeof *image32);
  unsigned char *p = image32;
  for (i = 0; i < width; ++i) {
    for (j = 0; j < height; ++j) {
        *p++ = i % 256; // blue
        *p++ = j % 256; // green
        if (i < 256) {
          *p++ = i % 256; // red
        } else if (j < 256) {
          *p++ =j % 256; // red
        } else {
          *p++ = (256 - j) % 256; // red
        }
      p++;
    }
  }
  return  XCreateImage(display, visual, 24, ZPixmap, 0,
                               (char *)image32, width, height, 32, 0);
}
#endif // WITH_STBI

void *openx(void *arg)
{
  int old_cancel_type;
  pthread_cleanup_push(closex, NULL);
  int width = 512, height = 512;
  XSetWindowAttributes attr;
  attr.event_mask =  ExposureMask; //  | KeyPressMask |PropertyChangeMask | StructureNotifyMask
  attr.override_redirect = True;
  Visual *visual = DefaultVisual(display, 0);

  Window top_window; // term_window
  top_window = get_focus_window(display);
  target_win = get_toplevel_parent(display, top_window);
  top_window = get_top_window(display, top_window);

#if defined(WITH_STBI)
  if (!XGetWindowAttributes(display, top_window, &xwa)) {
    PRINTWRITE("Error XGetWindowAttributes\n");
  }
  load_stbi();
  int x_dist = 100;
  int center_in_second_window_dist = xwa.width / 4;
#endif // WITH_STBI

  img_window = XCreateWindow(display, RootWindow(display, 0),
#if defined(WITH_STBI)
                             ((xwa.width / 2) + xwa.x + center_in_second_window_dist - _image->new_width / 2) - 10,
                             xwa.y + 49,
                             _image->new_width, _image->new_height,
#else
                             820, 10,
                             width, height,
#endif // WITH_STBI
                             0, 24, InputOutput, visual,
                             CWEventMask | CWOverrideRedirect,
                             &attr);

  set_window_properties(display, img_window);
  Atom WM_message[2];
  // Subscribe to window closing event
  WM_message[0] = XInternAtom(display, "WM_PROTOCOLS", 1);
  WM_message[1] = XInternAtom(display, "WM_DELETE_WINDOW", 1);
  XSetWMProtocols(display, img_window, WM_message, 2);
  if (visual->class != TrueColor) {
    write_line_debug(STDERR_FILENO, "Cannot handle non true color visual ...\n");
    exit(1);
  }

#if defined(WITH_STBI)
  put_image(img_window, ((xwa.width / 2) + xwa.x + center_in_second_window_dist - _image->new_width / 2) - 10, xwa.y + 49);

  _image->ximage = XCreateImage(display, CopyFromParent, _image->depth, ZPixmap,
                                0, (char *)_image->data_resized, _image->new_width, _image->new_height,
                                _image->bpl * 8, _image->bpl * _image->new_width);
#else
  _image->ximage = CreateTrueColorImage(display, visual, 0, width, height);
#endif // WITH_STBI

  //XSelectInput(display, top_window, KeyPressMask | ExposureMask | PropertyChangeMask | StructureNotifyMask);
  XSelectInput(display, top_window, ExposureMask | PropertyChangeMask | StructureNotifyMask);
  XMapRaised(display, img_window);
  raised = 1;
  mapped = 1;

  PRINTVALUE_UNSIGNED(img_window);

  int nth_call = 0;
  Atom_Prop atom_prop = { 0 };
  while (processEvent(display,
                      &top_window,
                      &img_window,
                      _image->ximage,
#if defined(WITH_STBI)
                      _image->new_width,
                      _image->new_height,
#else
                      width,
                      height,
#endif // WITH_STBI
                      WM_message,
                      &nth_call, &atom_prop)) {}
  // put the thread in deferred cancellation mode.
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old_cancel_type);

  // supplying '1' means to execute the cleanup handler
  // prior to unregistering it. supplying '0' would
  // have meant not to execute it.
  pthread_cleanup_pop(1);

  // restore the thread's previous cancellation mode.
  pthread_setcanceltype(old_cancel_type, NULL);
  pthread_exit(NULL);
}

int set_img(char *path)
{
  XInitThreads();
#if defined(WITH_STBI)
  unsigned int len_path = strlen(path);
  copy(&image_path, path, len_path);
#endif // WITH_STBI
  size_t sz_image = 1;
  CALLOC(_image, sz_image);
  display = XOpenDisplay(NULL);
  if (display == NULL) {
    write_line_debug(STDERR_FILENO, "cannot connect to X server\n");
    exit(1);
  }
  long i_ = 0;
  if (pthread_create(&keypress_thread_id, NULL, detect_keypress, NULL) < 0) {
    PRINTWRITE("Error thread\n");
  }
  if (pthread_create(&memory_thread_id, NULL, openx, (void *)i_) < 0) {
    PRINTWRITE("Error thread\n");
  }

  int c;
  for (;;) {
    c = kbget();
    threadInfo_t *t = &threads[0];           // get shorthand ptr to thread
    if (c == 'w' || c == 'q' || c == KEY_DOWN || c == KEY_UP) {
      pthread_cancel(memory_thread_id);
      break;
    }
    if (strchr(t->keys, c) != NULL) {        // this thread listens for this key
      pthread_mutex_lock(&t->kqmutex);       // lock other threads out while we tamper
      keyQueue_t *kq = calloc(sizeof (struct keyQueue), 1); // allocate pkt
      kq->key = c;                           // stash key there
      keyQueue_t *kptr = &t->kqhead;         // get pointer to queue head
      while (kptr->next != NULL)             // find first empty slot
        kptr = kptr->next;
      kptr->next = kq;                       // enqueue key packet to thread
      pthread_mutex_unlock(&t->kqmutex);     // unlock key queue
    }
  }

#if defined(EBUG)
  sleep(5); // you will see closex
#endif // EBUG
  return c;
}

/*
int main(int argc, char **argv)
{
#if defined(WITH_STBI)
  if (argc != 2) {
    write_line_debug(STDERR_FILENO, "Usage: ");
    PRINTWRITE(argv[0]); PRINTWRITE(" <IMAGE_PATH>\n");
    exit(1);
  }
#endif // WITH_STBI

  int c = set_img(argv[1]);
  write_line_debug(__file_descriptor, "key pressed: ");
  write_char(c); LINE_CH;
*/
/*
  size_t sz_image = 1;
  CALLOC(_image, sz_image);
  display = XOpenDisplay(NULL);
  if (display == NULL) {
    write_line_debug(STDERR_FILENO, "cannot connect to X server\n");
    exit(1);
  }
  long i_ = 0;
  if (pthread_create(&keypress_thread_id, NULL, detect_keypress, NULL) < 0) {
    PRINTWRITE("Error thread\n");
  }
  if (pthread_create(&memory_thread_id, NULL, openx, (void *)i_) < 0) {
    PRINTWRITE("Error thread\n");
  }

  //pthread_join(keypress_thread_id, NULL);
  //pthread_join(memory_thread_id, NULL);
  //pthread_detach(keypress_thread_id);
  //pthread_detach(memory_thread_id);

//
//  tcgetattr(0, &oterm);                         // get orig tty settings
//  term = oterm;                                // copy them
//  term.c_lflag &= ~ICANON;                      // put in '1 key mode'
//  term.c_lflag &= ~ECHO;                        // turn off echo
//
  for (;;) {
//
//    tcsetattr(0, TCSANOW, &term);             // echo off 1-key read mode
//    char c = getchar();                        // get single key immed.
//    tcsetattr(0, TCSANOW, &oterm);            // settings back to normal
//
    int c = kbget();
    write_line_debug(__file_descriptor, "key pressed: ");
    write_char(c); LINE_CH;
    threadInfo_t *t = &threads[0];           // get shorthand ptr to thread
    if (c == 'w' || c == 'q' || c == KEY_DOWN || c == KEY_UP) {
      pthread_cancel(memory_thread_id);
      break;
    }
    if (strchr(t->keys, c) != NULL) {        // this thread listens for this key
      pthread_mutex_lock(&t->kqmutex);       // lock other threads out while we tamper
      keyQueue_t *kq = calloc(sizeof (struct keyQueue), 1); // allocate pkt
      kq->key = c;                           // stash key there
      keyQueue_t *kptr = &t->kqhead;         // get pointer to queue head
      while (kptr->next != NULL)             // find first empty slot
        kptr = kptr->next;
      kptr->next = kq;                       // enqueue key packet to thread
      pthread_mutex_unlock(&t->kqmutex);     // unlock key queue
    }
  }
*/
/*
#if defined(EBUG)
  sleep(5); // you will see closex
#endif // EBUG
  return 0;
}
*/
