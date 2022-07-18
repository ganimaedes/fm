#include "props.h"
#include "draw.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <signal.h>
#include <pthread.h>
#include <signal.h>
#include <execinfo.h>
#include <errno.h>
#include <semaphore.h>

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
volatile sig_atomic_t lower_window = 0;
volatile sig_atomic_t close_prog = 0;
//volatile sig_atomic_t resized = 0;
volatile sig_atomic_t second_loop = 0;

threadInfo_t threads[] = { { PTHREAD_MUTEX_INITIALIZER, { NULL, '\0' }, "q" } };
static Image *_image;
static unsigned int total_len = 0;
pthread_t keypress_thread_id, memory_thread_id;
static char *image_path = NULL;
Window top_window; // term_window
Visual *visual;
XSetWindowAttributes attr;
Atom WM_message[2]; // Subscribe to window closing event
int keypressed = 0;
int fds[2];
sem_t mutex;
Atom_Prop atom_prop = { 0 };

pthread_mutex_t winchange_mutex = PTHREAD_MUTEX_INITIALIZER;

volatile sig_atomic_t done_program;

struct _global {
  int pip[2];     // extra pipe for event loop
  sig_atomic_t done;  // flag to stop the event loop
} global;

fd_set set_read, set_save;

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
  Atom win_type_notif[] = { XInternAtom(dsp, "_NET_WM_WINDOW_TYPE_NOTIFICATION", False), };
  XChangeProperty(dsp, win, win_type, XA_ATOM, 32,
                  PropModeReplace, (unsigned char *)&win_type_notif, 1);

  // always on top, not in taskbar or pager.
  Atom win_state = XInternAtom(dsp, "_NET_WM_STATE", False);
  Atom win_state_setting[] =
  {
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
  write_line_debug(file_descriptor, "----------Map Notify\n");
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
int check_event(Display *display, XEvent *local_event, Window *img_window, Atom_Prop *atom_prop)
{
  switch (local_event->type) {
    case ConfigureNotify:
      if (mapped == 0 && just_unmapped == 1) {
        XMapRaised(display, *img_window);
        mapped = 1;
        just_unmapped = 0;
        config_check_event_mapped = 1;
      }
#if defined(V_DEBUG)
      __PRINTDEBUG; __PRINTMAPINFO2("ConfigureNotify"); //__PRINTEVENTSELECTIONS;
#endif
      if (!XGetWindowAttributes(display, top_window, &xwa_new)) {
        PRINTWRITE("Error XGetWindowAttributes\n");
      }
      if (xwa_new.width != xwa.width || xwa_new.height != xwa.height) {
#if defined(V_DEBUG)
      __PRINTDEBUG; __PRINTMAPINFO2("RESIZED");
#endif
        return 0;
      }
      break;
    case MotionNotify:
#if defined(V_DEBUG)
      __PRINTDEBUG; __PRINTMAPINFO2("Motion Notify");
#endif
      //  motion_handler(local_event, display);
      break;
    case CreateNotify:
      break;
    case MapNotify:
      // here MapNotify applies to term_window so we map the img_window as well
      XMapRaised(display, *img_window);
#if defined(V_DEBUG)
      __PRINTDEBUG; __PRINTMAPINFO2("MapNotify");
#endif
      map_notify_handler(*local_event, display);
      break;
    case UnmapNotify:
      // here UnmapNotify applies to term_window so we unmap the img_window as well
      XUnmapWindow(display, *img_window);
#if defined(V_DEBUG)
      __PRINTDEBUG; __PRINTMAPINFO2("UnMapNotify");
#endif
      break;
    case DestroyNotify:
#if defined(V_DEBUG)
      __PRINTDEBUG; __PRINTMAPINFO2("Destroy Event");
#endif
      break;
    case ButtonPress:
#if defined(V_DEBUG)
      __PRINTDEBUG; __PRINTMAPINFO2("Event button pressed");
#endif
      break;
    case KeyPress:
#if defined(V_DEBUG)
      __PRINTDEBUG; __PRINTMAPINFO2("Keyboard key pressed");
#endif
      break;
    case FocusIn:
      if (mapped == 0 || just_unmapped == 1) {
        XMapRaised(display, *img_window);
        mapped = 1;
        just_unmapped = 0;
      }
#if defined(V_EBUG)
      __PRINTDEBUG; __PRINTMAPINFO2("FocusIn");
#endif // V_EBUG
      break;
    case FocusOut:
      if (mapped == 1 && just_unmapped == 0) {
        XUnmapWindow(display, *img_window);
#if defined(WITH_PROPS)
        if (lower_window && strstr(atom_prop->status, "HIDDEN")) {
          XLowerWindow(display, *img_window);
        }
#endif // WITH_PROPS
        mapped = 0;
        just_unmapped = 1;
      }
#if defined(V_EBUG)
      __PRINTDEBUG; __PRINTMAPINFO2("FocusOut");
#endif // V_EBUG
      break;
    default:
#if defined(V_EBUG)
      __PRINTDEBUG; __PRINTMAPINFO2("default");
#endif // V_EBUG
      break;
  }
  return 1;
}

int get_cursor_position(int ifd, int ofd, int *rows, int *cols)
{
  wchar_t buffer[32];
  unsigned int i = 0;

  if (write(ofd, "\033[6n", 4) != 4) { return -1; }

  while (i < sizeof(buffer) - 1) {
    if (read(ifd, buffer + i, 1) != 1) { break; }
    if (buffer[i] == 'R') { break; }
    ++i;
  }
  buffer[i] = '\0';

  if (buffer[0] != KEY_ESCAPE || buffer[1] != '[') { return -1; }
  if (sscanf((char *)buffer + 2, "%d;%d", rows, cols) != 2) { return -1; }
  return 0;
}

int get_window_size(int ifd, int ofd, int *rows, int *cols)
{
  struct winsize w_s;

  if (ioctl(1, TIOCGWINSZ, &w_s) == -1 || w_s.ws_col == 0) {
    int orig_row = 0, orig_col = 0, retval = 0;

    retval = get_cursor_position(ifd, ofd, rows, cols);
    if (retval == -1) { return -1; }

    if (write(ofd, "\033[999C\033[999B", 12) != 12) { return -1; }
    retval = get_cursor_position(ifd, ofd, rows, cols);
    if (retval == -1) { return -1; }

    char seq[32];
    snprintf((char *)seq, 32, "\033[%d;%dH", orig_row, orig_col);
    if (write(ofd, (char *)seq, strlen(seq)) == -1) {
      return -1; //  Can't recover
    }
    return 0;
  } else {
    *rows = w_s.ws_row;
    *cols = w_s.ws_col;
    return 0;
  }
  return -1;
}

int getch(void)
{
  int c = 0;
  tcgetattr(STDIN_FILENO, &oterm);
  memcpy(&term, &oterm, sizeof(term));
  term.c_lflag &= ~(ICANON | ECHO);
  term.c_cc[VMIN] = 1;
  term.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
  c = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oterm);
  return c;
}

int kbhit(void)
{
  int c = 0;

  tcgetattr(STDIN_FILENO, &oterm);
  memcpy(&term, &oterm, sizeof(term));
  term.c_lflag &= ~(ICANON | ECHO);
  term.c_cc[VMIN] = 0;
  term.c_cc[VTIME] = 1;
  tcsetattr(STDIN_FILENO, TCSANOW, &term);
  c = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oterm);
  if (c != -1) { ungetc(c, stdin); }
  return c != -1 ? 1 : 0;
}

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
    write_line_debug(file_descriptor, "Error occured while reset : errno = ");
  }
}

int kbesc(void)
{
  int c = 0;

  if (!kbhit()) { return KEY_ESCAPE; }
  c = getch();
  if (c == BACKSPACE) { ungetc(c, stdin); return c; }
  if (c == '[') {
    switch ((c = getch())) {
      case 'A':
        c = KEY_UP;
        //c = UP;
        break;
      case 'B': {
        c = KEY_DOWN;
        break;
      }
      case 'C':
        c = KEY_LEFT;
        break;
      case 'D':
        c = KEY_RIGHT;
        break;
      case '3':
        c = KEY_SUPPR;
        break;
      case 'F':
      case '4':
        c = KEY_END;
        break;
      case 'H':
        c = KEY_HOME;
        break;
      case '5':
        c = KEY_PAGE_UP;
        break;
      case '6':
        c = KEY_PAGE_DN;
        break;
      default:
        c = 0;
        break;
    }
  } else {
    c = 0;
  }
  if (c == 0) { while (kbhit()) { getch(); } }
  return c;
}

int kbget(void)
{
  int c = getch();
  return c == KEY_ESCAPE ? kbesc() : c;
}

void signal_handler(int n)
{
  //write(global.pip[1], &global.done, 1);  // wake up select in X11 event loop
  switch (n) {
    case SIGUSR1:                // user pressed ctrl-c from terminal (or kill -s SIGINT)
      global.done = 1;            // tell event loop to exit
  }
}

void initialize()
{
  struct sigaction sigact;        // no race conditions like signal()
  memset(&sigact, 0, sizeof(sigact));
  sigact.sa_handler = &signal_handler;
  sigaction(SIGUSR1, &sigact, NULL);
  memset(&global, 0, sizeof(global));
  global.done = 0;
  //pipe(global.pip);
  //write(global.pip[1], &global.done, 1);  // wake up select on first iteration of X11 event loop
}

int processEvent3(Display *display,
                 Window *img_window,
                 int width,
                 int height,
                 Atom WM_message[2],
                 int *nth_call_to_process_event_function, Atom_Prop *atom_prop)
{
  XEvent ev;
// https://stackoverflow.com/questions/12871071/x11-how-to-delay-repainting-until-all-events-are-processed
#if defined(WITH_PROPS)
  show_properties(atom_prop, img_window, display, 1);
  PRINTSTRING(atom_prop->status);
  //show_properties(atom_prop, top_window, display, 1);
#endif // WITH_PROPS

  if (XNextEvent(display, &ev) >= 0) {
    XLockDisplay(display);
    sem_wait(&mutex);

    XSelectInput(display, top_window,
         ExposureMask | PropertyChangeMask | StructureNotifyMask | FocusChangeMask | LeaveWindowMask);
    // https://stackoverflow.com/questions/3806872/window-position-in-xlib
    int x = 0, y = 0;
    XWindowAttributes xwa;
    Window child;
    XTranslateCoordinates(display, top_window, *img_window, 0, 0, &x, &y, &child);
    if (check_event(display, &ev, img_window, atom_prop) == 0) {
      XUnlockDisplay(display);
      pthread_mutex_lock(&winchange_mutex);
      global.done = 1;
      pthread_mutex_unlock(&winchange_mutex);
      sem_post(&mutex);
      usleep((rand() / (1.0 + RAND_MAX)) * 1000000);
      return 0;
    }
    switch (ev.type) {
      case Expose:
        XPutImage(display, *img_window, _image->gc, _image->ximage, 0, 0, 0, 0, _image->new_width, _image->new_height);
        ++*nth_call_to_process_event_function;
        XUnlockDisplay(display);
        sem_post(&mutex);
        return 1;
      case ClientMessage:
        if (ev.xclient.message_type == WM_message[0]) {
          if (ev.xclient.data.l[0] == WM_message[1]) {
            XUnlockDisplay(display);
            sem_post(&mutex);
            return 0;
          }
        }
      case ConfigureNotify:
#if defined(V_EBUG)
        __PRINTDEBUG;
#endif // V_EBUG
        ++*nth_call_to_process_event_function;
        if (mapped == 1) {
          XUnmapWindow(display, *img_window);
          mapped = 0;
          just_unmapped = 1;
#if defined(V_EBUG)
          __PRINTMAPINFO_N2("ConfigureNotify");
#endif // V_EBUG
          //just_unraised = 1;
        } else if (mapped == 0) {
          XMapRaised(display, *img_window);
          mapped = 1;
          just_unmapped = 0;
#if defined(V_EBUG)
          __PRINTMAPINFO_N2("ConfigureNotify");
#endif // V_EBUG
        }
        XUnlockDisplay(display);
        sem_post(&mutex);
        return 1;
        break;
      default:
        if (second_loop == 1) {
          pthread_t tid;
          read(fds[0], &tid, sizeof(tid));
          if (fds[0] == keypress_thread_id) {
#if defined(V_EBUG)
            PRINTWRITE("fds[0] == keypress_thread_id");
#endif // V_EBUG
            sem_post(&mutex);
            XUnlockDisplay(display);
            return 0;
          }
          if (mapped == 0 || just_unmapped == 1) {
            XMapRaised(display, *img_window);
            //raised = 1;
            just_unmapped = 0;
            mapped = 1;
            second_loop = 0;
            ++*nth_call_to_process_event_function;
#if defined(V_EBUG)
            __PRINTDEBUG; __PRINTMAPINFO2("default");
#endif // V_EBUG
            XUnlockDisplay(display);
            sem_post(&mutex);
            return 1;
          } else if (mapped == 1 && just_unmapped == 0) {
#if defined(V_EBUG)
            __PRINTDEBUG;
            write_line_debug(file_descriptor, "ISMAPPED\n");
#endif // V_EBUG
          }
        }
        XUnlockDisplay(display);
        sem_post(&mutex);
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
  }
  return w;
}

Window get_focus_window(Display* d)
{
  Window w;
  int revert_to;
  XGetInputFocus(d, &w, &revert_to);
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

// if 'q' is pressed quit the memory_thread ie stop allocating memory and free allocated memory
void *detect_keypress(void *arg)
{
  char key;
  threadInfo_t *t = &threads[0];    // get pointer to thread
  pthread_t t_hread = pthread_self();
  for (;;) {
    pthread_mutex_lock(&t->kqmutex);     // lock other threads out while we tamper
    key = '\0';                          // initialize key to NULL
    if (t->kqhead.next != NULL) {        // Anything queued up for us?
      //usleep((rand() / (1.0 + RAND_MAX)) * 1000000);
      usleep((rand() / (1.0 + RAND_MAX)) * 1000000);
      keyQueue_t *kq = t->kqhead.next; // if so get ptr to key pkt
      key = kq->key;                   // fetch key from pkt
      PRINTVALUE(key);
      t->kqhead.next = kq->next;       // Point to next key in queue (or NULL if no more queued up).
      free(kq);
      pthread_mutex_unlock(&t->kqmutex);   // unlock key queue
      usleep(1000);
      pthread_cancel(memory_thread_id);
    }
    pthread_mutex_unlock(&t->kqmutex);   // unlock key queue
    if (key != '\0') {                   // if we got a key, log it
      //write_line("pulled key "); write_line(&key); write_line(" from queue\n");
    }
#if defined(V_EBUG)
      write(fds[1], &t_hread, sizeof(t_hread));
      PRINTVALUE(fds[1]);
#endif // V_EBUG
    usleep(1000);
  }
  pthread_exit(NULL);
}

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
#if defined(EBUG)
  PRINTWRITE(type);
#endif // EBUG
quit_file_type:
  fseek(info->file, 0, SEEK_SET);
  fclose(info->file);
  return type;
}

void closex(void *arg)
{
//  if (close_prog == 0) {
    sem_wait(&mutex);
    XLockDisplay(display);
#if defined(WITH_STBI)
    if (_image->data != NULL) {
#if defined(V_DEBUG)
      PRINTWRITE("trying stbi_image_free(_image->data)");
#endif // V_DEBUG
      stbi_image_free(_image->data);
      _image->data = NULL;
#if defined(V_DEBUG)
      PRINTWRITE("stbi_image_free(_image->data) called");
#endif // V_DEBUG
    }
    if (_image->ximage != NULL) {
#if defined(V_DEBUG)
      PRINTWRITE("Trying XDestroyImage(_image->ximage)");
#endif // V_DEBUG
      XDestroyImage(_image->ximage);
      _image->ximage = (XImage *)NULL;
#if defined(V_DEBUG)
      PRINTWRITE("XDestroyImage(_image->ximage) called");
#endif // V_DEBUG
    }
    sem_post(&mutex);
    if (_image->gc != NULL) {
      sem_wait(&mutex);
#if defined(V_DEBUG)
      PRINTWRITE("Trying XFreeGC(display, _image->gc)");
#endif // V_DEBUG
      XFreeGC(display, _image->gc);
      _image->gc = (GC *)NULL;
#if defined(V_DEBUG)
      PRINTWRITE("XFreeGC(display, _image->gc) called");
#endif // V_DEBUG
      sem_post(&mutex);
    }
    usleep(90000);
    usleep(90000);
    if (_image->data != NULL) {
      sem_wait(&mutex);
#if defined(V_DEBUG)
      PRINTWRITE("Trying stbi_image_free(_image->data)");
#endif // V_DEBUG
      stbi_image_free(_image->data);
      _image->data = NULL;
#if defined(V_DEBUG)
      PRINTWRITE("stbi_image_free(_image->data) called");
#endif // V_DEBUG
      sem_post(&mutex);
    }
#endif // WITH_STBI
       ///*
    if (img_window != 0) {
      sem_wait(&mutex);
#if defined(V_DEBUG)
      PRINTWRITE("Trying XDestroyWindow(display, img_window)");
#endif // V_DEBUG
#if defined(WITH_PROPS)
      show_properties(&atom_prop, &img_window, display, 1);
      PRINTSTRING(atom_prop.status);
      if (strlen(atom_prop.status) > 0) {
        PRINTWRITE("strlen(atom_prop.status) > 0");
      }
#endif // WITH_PROPS
      XDestroyWindow(display, img_window);
#if defined(V_DEBUG)
      PRINTWRITE("XDestroyWindow(display, img_window) called");
#endif // V_DEBUG
      sem_post(&mutex);
    }
    //*/
    usleep(90000);
    if (image_path != NULL) {
      sem_wait(&mutex);
#if defined(V_DEBUG)
      PRINTWRITE("trying free(image_path)");
#endif // V_DEBUG
      free(image_path);
      image_path = NULL;
#if defined(V_DEBUG)
      PRINTWRITE("free(image_path) called");
#endif // V_DEBUG
      sem_post(&mutex);
    }
    if (_image != NULL) {
      sem_wait(&mutex);
#if defined(V_DEBUG)
      PRINTWRITE("Trying free(_image)");
#endif // V_DEBUG
      free(_image);
      _image = NULL;
#if defined(V_DEBUG)
      PRINTWRITE("free(_image) called");
#endif // V_DEBUG
      sem_post(&mutex);
    }
    //sleep(1);
    usleep(10000);
    sem_post(&mutex);
    XUnlockDisplay(display);
    if (display != NULL) {
#if defined(V_DEBUG)
      PRINTWRITE("Trying XCloseDisplay(display)");
#endif // V_DEBUG
      XCloseDisplay(display);
#if defined(V_DEBUG)
      PRINTWRITE("XCloseDisplay(display) called");
#endif // V_DEBUG
    }
//  }
  //pthread_t tid;
  //read(fds[0], &tid, sizeof(tid));
}

double fix_factor_to_fit_inside_window(Image *img, int width, int height)
{
  double factor = 1.0;
  double upper_lower_limit = _image->win_lower_limit;
  if (img->width > (width / _image->n_windows_in_fm) - _image->win_top_limit) {
    factor = 1.0 / (((double)img->width) / ((double)((width / _image->n_windows_in_fm) - _image->win_top_limit)));
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
  //factor = imageinfo->factor;
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

void resize_image()
{
  if (!XGetWindowAttributes(display, top_window, &xwa)) {
    PRINTWRITE("Error XGetWindowAttributes\n");
  }

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
}

void get_bpl()
{
  // https://stackoverflow.com/questions/11069666/cannot-get-xcreatesimplewindow-to-open-window-at-the-right-position
  _image->bpl = 0;
  nanosleep((const struct timespec[]){{0, 500000L}}, NULL);
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

  _image->new_width = 512;
  _image->new_height = 512;

  attr.event_mask =  ExposureMask; //  | KeyPressMask |PropertyChangeMask | StructureNotifyMask
  attr.override_redirect = True;
  visual = DefaultVisual(display, 0);
  if (visual->class != TrueColor) {
    write_line_debug(STDERR_FILENO, "Cannot handle non true color visual ...\n");
    exit(1);
  }

  top_window = get_focus_window(display);
  top_window = get_top_window(display, top_window);
  if (!XGetWindowAttributes(display, top_window, &xwa)) {
    PRINTWRITE("Error XGetWindowAttributes\n");
  }
#if defined(EBUG)
  PRINTVALUE_UNSIGNEDLONG(top_window);
#endif // EBUG

#if defined(WITH_STBI)

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
///*
  if (_image->data != NULL) {
#if defined(EBUG)
    PRINTWRITE("trying stbi_image_free(_image->data)");
#endif // EBUG
    stbi_image_free(_image->data);
    _image->data = NULL;
#if defined(EBUG)
    PRINTWRITE("stbi_image_free(_image->data) called");
#endif // EBUG
  }
//*/
  //int center_in_second_window_dist = xwa.width / 4;
  int center_in_second_window_dist = xwa.width / (_image->n_windows_in_fm * 2);
  double x = ((double)xwa.width / _image->n_windows_in_fm);
  double x_pos = xwa.width / 2;
  if (_image->n_windows_in_fm > 2) {
    x_pos = 2 * (xwa.width / 3);
  }
#endif // WITH_STBI


  img_window = XCreateWindow(display, RootWindow(display, 0),
#if defined(WITH_STBI)
                             (x_pos + xwa.x + center_in_second_window_dist - _image->new_width / 2) - 10, _image->win_top_limit,
#else
                             820, 10,
#endif // WITH_STBI
                             _image->new_width, _image->new_height,
                             0, 24, InputOutput, visual,
                             CWEventMask | CWOverrideRedirect /* | CWBitGravity */,
                             &attr);

  _image->gc = XCreateGC(display, img_window, 0, 0);

#if defined(EBUG)
  PRINTVALUE_UNSIGNEDLONG(img_window);
#endif // EBUG
  set_window_properties(display, img_window);
  WM_message[0] = XInternAtom(display, "WM_PROTOCOLS", 1);
  WM_message[1] = XInternAtom(display, "WM_DELETE_WINDOW", 1);
  XSetWMProtocols(display, img_window, WM_message, 2);

#if defined(WITH_STBI)
  XSizeHints    my_hints = {0};
  my_hints.flags  = PPosition | PSize;     // specify position and size
  my_hints.x      = ((xwa.width / 2) + xwa.x + center_in_second_window_dist - _image->new_width / 2) - 10;
  my_hints.y      = xwa.y + _image->win_top_limit;
  my_hints.width  = _image->new_width;
  my_hints.height = _image->new_height;
  XSetNormalHints(display, img_window, &my_hints);  // Where new_window is the new window
  get_bpl();

  _image->ximage = XCreateImage(display, CopyFromParent, _image->depth, ZPixmap,
                                0, (char *)_image->data_resized, _image->new_width, _image->new_height,
                                _image->bpl * 8, _image->bpl * _image->new_width);
#else
  _image->ximage = CreateTrueColorImage(display, visual, 0, _image->new_width, _image->new_height);
#endif // WITH_STBI

  XSelectInput(display, top_window, ExposureMask | PropertyChangeMask | StructureNotifyMask);
  XMapRaised(display, img_window);
  raised = 1;
  mapped = 1;

#if defined(EBUG)
  PRINTVALUE_UNSIGNEDLONG(img_window);
#endif // EBUG

  int nth_call = 0;
  struct winsize w_s;
  while ((ioctl(0, TIOCGWINSZ, &w_s)) ||
         processEvent3(display,
                      &img_window,
                      _image->new_width,
                      _image->new_height,
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

static void sig_win_ch_handler(int sig) { resized = 1; }

void init_win_change_signal()
{
  struct sigaction sact;
  sigemptyset(&sact.sa_mask);
  sact.sa_flags = 0;
  sact.sa_handler = sig_win_ch_handler;
  char err1[] = "sigaction";
  if (sigaction(SIGWINCH, &sact, NULL)) {
    write(2, err1, sizeof(err1));
  }
}

int set_img(char *path)
{
#if defined(WITH_STBI)
  unsigned int len_path = strlen(path);
  copy(&image_path, path, len_path);
#endif // WITH_STBI

  srand(time(0));
  pipe(fds);
  if (pthread_create(&keypress_thread_id, NULL, detect_keypress, NULL) < 0) {
    PRINTWRITE("Error thread\n");
  }
  pthread_detach(keypress_thread_id);
  if (pthread_create(&memory_thread_id, NULL, openx, NULL) < 0) {
    PRINTWRITE("Error thread\n");
  }
  pthread_detach(memory_thread_id);

  // https://stackoverflow.com/questions/2889415/in-xlib-how-can-i-animate-until-an-event-occurs

  global.done = 0;
  int c = 0;
  for (;;) {

    if (resized == 0) {
      c = kbget();
    }
    sem_wait(&mutex);
    pthread_mutex_lock(&winchange_mutex);
    pthread_mutex_unlock(&winchange_mutex);
    sem_post(&mutex);
    if (c == KEY_DOWN || c == KEY_UP || c == KEY_ESCAPE || c == KEY_BACKSPACE ||
        c == KEY_PAGE_DN || c == KEY_PAGE_UP || c == KEY_BACK || c == KEY_END || c == KEY_HOME ||
        global.done == 1 || resized == 1 || c == 'a' || c == 'r' || c == KEY_Q) {
      keypressed = 1;
      pthread_cancel(memory_thread_id);
      break;
    }
  }

  // if uncomment next line and q pressed before image is loaded it causes segfault error in xcb
  if (keypressed == 0) {
    pthread_join(memory_thread_id, NULL);
  }
  return c;
}

void handler(int sig)
{
  void *array[10];
  size_t size = backtrace(array, 10);
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

int main(int argc, char **argv)
{
//  usleep(90000);
#if defined(WITH_STBI)
  if (argc != 5 && (quit = TRUE)) {
    write_line_debug(STDERR_FILENO, "Usage: ");
    write_line(argv[0]); PRINTWRITE(" <IMAGE_PATH> <Window Top Limit> <Window Left Begin> <Window Lower Limit>");
    exit(1);
  }
#endif // WITH_STBI
  STAT_INFO stat_info = { 0 };
  unsigned int len_argv1 = strlen(argv[4]);
  copy(&stat_info.file_name, argv[4], len_argv1);
  if (find_file_type2(&stat_info) == 0) {
    write_line(argv[4]); write_line_debug(STDERR_FILENO, " is not an image\n");
    free(stat_info.file_name);
    stat_info.file_name = NULL;
    exit(1);
  }
  free(stat_info.file_name);
  stat_info.file_name = NULL;

  size_t sz_image = 1;
  CALLOC(_image, sz_image);

  XInitThreads();
  signal(SIGSEGV, handler);
  init_win_change_signal();

  display = XOpenDisplay(NULL);
  if (display == NULL) {
    write_line_debug(STDERR_FILENO, "cannot connect to X server\n");
    exit(1);
  }

  _image->win_top_limit = (double)atof(argv[1]);
  _image->n_windows_in_fm = (double)atof(argv[2]);
  _image->win_lower_limit = (double)atof(argv[3]);

  sem_init(&mutex, 0, 1);
  int c = 0;
  c = set_img(argv[4]);
  //PRINTVALUE_1LINE(c); LINE_CH;
  PRINT_INTEGER(c);
  //ttymode_reset(ECHO, 1);

#if defined(WITH_PROPS)
  if (atom_prop.status != NULL) {
    free(atom_prop.status);
    atom_prop.status = NULL;
  }
#endif // WITH_PROPS
#if defined(EBUG)
  write_line_debug(file_descriptor, "key pressed: ");
  write_char(c); LINE_CH;
#endif // EBUG
  usleep(9000);
  return c;
}
