#include "scr.h"
#include <stdio.h>
#include <unistd.h>

int identify_set_keys(char *keys, int starting_pos)
{
  int c;
  if (keys[starting_pos + 1] == '[') {
    switch (keys[starting_pos + 2]) {
      case 'A':
        c = KEY_UP;
        //c = UP;
        break;
      case 'B':
        c = KEY_DOWN;
        break;
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
  return c;
}

int get_char()
{
  tcgetattr(STDIN_FILENO, &oterm);
  term = oterm;
  term.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &term);

  char keys[4];
  int nbytes;
  int key_pressed;
  while ((nbytes = read(STDIN_FILENO, keys, sizeof(keys))) > 0) {
    for (int i = 0; i < nbytes; ++i) {
      char key = keys[i];
      if (key == EOF_KEY) {
        fprintf(stderr, "%d (control-D or EOF)\n", key);
        goto end_loops;
      } else if (key == KEY_ESCAPE && nbytes > i + 1) {
        char *keys_copy = keys;
        key_pressed = identify_set_keys(keys_copy, i);
        //PRINTINT(key_pressed);
        if (key_pressed == KEY_ESCAPE || key_pressed == KEY_UP || key_pressed == KEY_DOWN ||
            key_pressed == KEY_HOME || key_pressed == KEY_END || key_pressed == KEY_PAGE_UP ||
            key_pressed == KEY_PAGE_DN) {
          //PRINTINT(key_pressed);
          goto end_loops;
          break;
        }
        break;
      } else {
        key_pressed = key;
        goto end_loops;
        //printf("%i\n", (int)key);
      }
      if (key == 'q') {
        key_pressed = KEY_Q;
        goto end_loops;
      } else {
        key_pressed = key;
        goto end_loops;
      }
    }
  }

end_loops:
  tcsetattr(STDIN_FILENO, TCSANOW, &oterm);
  return key_pressed;
}

char m_getch()
{
  char buf = 0;
  struct termios old = { 0 };
  if (tcgetattr(0, &old) < 0) {
    perror("tcsetattr()");
  }
  old.c_lflag &= ~ICANON;
  old.c_lflag &= ~ECHO;
  old.c_cc[VMIN] = 1;
  old.c_cc[VTIME] = 0;
  if (tcsetattr(0, TCSANOW, &old) < 0) {
    perror("tcsetattr ICANON");
  }
  if (read(0, &buf, 1) < 0) {
    perror ("read()");
  }
  old.c_lflag |= ICANON;
  old.c_lflag |= ECHO;
  if (tcsetattr(0, TCSADRAIN, &old) < 0) {
    perror ("tcsetattr ~ICANON");
  }
  return (buf);
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
    //if (errno == EINTR || errno == EAGAIN)
    //  continue;
    printf("Error occured while reset %x: errno=%d\n", mode, errno);
  }
}

//int kbesc(void)
long unsigned kbesc(void)
{
  int c = 0;

  if (!kbhit()) { return KEY_ESCAPE; }
  //if (!kbhit()) { return 0; }
  //if (!kbhit()) { return KEY_Q; }
  c = getch();
  if (c == BACKSPACE) { ungetc(c, stdin); return c; }
  if (c == '[') {
    switch ((c = getch())) {
      case 'A':
        c = KEY_UP;
        //c = UP;
        break;
      case 'B':
        c = KEY_DOWN;
        break;
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
  // CTRL KEYS
  // CTRL+D ^D
  if (c == 4) {
    return c;
  } else if (c == 21) {
    return c;
  } else if (c == -1 /*|| c == KEY_ESCAPE*/) {
    ungetc(c, stdin);
    return 'w';
  }
  //return c == KEY_ESCAPE ? kbesc() : c;
  if (c == KEY_ESCAPE) {
    c = kbesc();
    if (c == KEY_ESCAPE) {
      ungetc(c, stdin);
      ttymode_reset(ECHO, 0);
      c = 'w';
    }
  }
  return c;
}

int kbesc2(void)
{
  int c = 0;

  if (!kbhit()) { return KEY_ESCAPE; }
  //if (!kbhit()) { return 113; }
  //if (!kbhit()) { return KEY_Q; }
  c = getch();
  if (c == BACKSPACE) { ungetc(c, stdin); return c; }
  if (c == '[') {
    switch ((c = getch())) {
      case 'A':
        c = KEY_UP;
        //c = UP;
        break;
      case 'B':
        c = KEY_DOWN;
        break;
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

int kbget2(void)
{
  int c = getch();
  // CTRL KEYS
  // CTRL+D ^D
  if (c == 4) {
    return c;
  } else if (c == 21) {
    return c;
  }
/*
  if (c == KEY_ESCAPE) {
    c = kbesc();
    TTYINTFD(1, 30, 1, c);
  } else {
    TTYINTFD(1, 30, 1, c);
    return c;
  }
  return c;
*/
  return c == KEY_ESCAPE ? kbesc() : c;
  //return c == 113 ? kbesc() : c;
  //sleep(5);
  //return c == KEY_Q ? kbesc() : c;
/*
  if (c == KEY_ESCAPE) {
    return c;
  } else {
    kbesc();
  }
*/
  //return c == KEY_Q ? kbesc() : c;
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

  //if (buffer[0] != KEY_ESCAPE || buffer[1] != '[') { return -1; }
  //if (buffer[0] != 'q' || buffer[1] != '[') { return -1; }
  if (buffer[0] != KEY_Q || buffer[1] != '[') { return -1; }
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
      //  Can't recover 
      return -1;
    }
    return 0;
  } else {
    *rows = w_s.ws_row;
    *cols = w_s.ws_col;
    return 0;
  }
  return -1;
}
