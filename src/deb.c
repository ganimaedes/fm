#include "deb.h"

#define ATTRIBUTES_NELEMENTS " attr_n_els: %d"
#define UPPER_POS            " upper_pos: %d"
#define LOWER_POS            " lower_pos: %d"
#define POS                  " pos: %d"

#if defined(EBUG)
int print_tty(Window *w, int fd, Attributes *attributes)
{
  if (attributes->n_elements == 0) {
    return 0;
  }

  struct winsize w_s;
  int i = 0;
  int to_print = attributes->n_elements;

  char attr_num_elements[sizeof(ATTRIBUTES_NELEMENTS)];
  char upper_pos[sizeof(UPPER_POS)];
  char lower_pos[sizeof(LOWER_POS)];
  char pos[sizeof(POS)];
  erase_scr(fd, "\033[2J");
  if (!ioctl(fd, TIOCGWINSZ, &w_s)) {
    if (w_s.ws_row < attributes->n_elements - 1) {
      to_print = w_s.ws_row;
    }
    for (i = 0; i < to_print; ++i) {
      sprintf(position, place, i + w->y_beg + 1, w->x_beg);
      move(fd, position);
      write(fd, attributes->paths[i], strlen(attributes->paths[i]));

      sprintf(attr_num_elements, ATTRIBUTES_NELEMENTS, attributes->n_elements);
      write(fd, attr_num_elements, strlen(attr_num_elements));

      sprintf(upper_pos, UPPER_POS, attributes->pos[i]->m_upper_pos);
      write(fd, upper_pos, strlen(upper_pos));

      sprintf(lower_pos, LOWER_POS, attributes->pos[i]->m_lower_pos);
      write(fd, lower_pos, strlen(lower_pos));

      sprintf(pos, POS, attributes->pos[i]->m_position);
      write(fd, pos, strlen(pos));
    }
    sprintf(position, place, i + w->y_beg, w->x_beg);
    move(fd, position);
  }
  return 1;
}
#endif // EBUG

#if defined(EBUG)
#define LENGTH_CHILD  " length_child: %lu"
#define LENGTH_PARENT " length_parent: %lu"
#define VALUE_CHILD   " value_child: %s"
#define VALUE_PARENT  " value_parent: %s"

int print_route_positions(Window *w, int fd, Array *left_box, char *parent, int *pos)
{

  struct winsize w_s;
  int i = 0;
  int to_print = 4;

  char length_child[sizeof(LENGTH_CHILD)];
  char length_parent[sizeof(LENGTH_PARENT)];
  char value_child[sizeof(VALUE_CHILD)];
  char value_parent[sizeof(VALUE_PARENT)];
  erase_scr(fd, "\033[2J");
  if (!ioctl(fd, TIOCGWINSZ, &w_s)) {
    if (w_s.ws_row < left_box->n_elements - 1) {
      to_print = w_s.ws_row;
    }

//    for (i = 0; i < to_print; ++i) {
      sprintf(position, place, i + w->y_beg + 1, w->x_beg);
      move(fd, position);
      //write(fd, "length = ", strlen("length = "));

      size_t len_child = strlen(left_box->menu[*pos].complete_path);
      sprintf(length_child, LENGTH_CHILD, len_child);
      write(fd, length_child, strlen(length_child));

      sprintf(position, place, i + w->y_beg + 2, w->x_beg);
      move(fd, position);

      //size_t j;
      //for (j = length_child - 1; j >= 0; --j) if (left_box->menu[*pos].complete_path[j] == '/') break;


      size_t len_parent = strlen(parent);
      sprintf(length_parent, LENGTH_PARENT, len_parent);
      write(fd, length_parent, strlen(length_parent));

      sprintf(position, place, i + w->y_beg + 3, w->x_beg);
      move(fd, position);

      sprintf(value_child, VALUE_CHILD, left_box->menu[*pos].complete_path);
      write(fd, value_child, strlen(value_child));

      sprintf(position, place, i + w->y_beg + 4, w->x_beg);
      move(fd, position);

      sprintf(value_parent, VALUE_PARENT, parent);
      write(fd, value_parent, strlen(value_parent));
    }
    sprintf(position, place, i + w->y_beg, w->x_beg);
    move(fd, position);
//  }

  return 1;
}
#endif // EBUG
