#include "parcours.h"

int num_of_slashes(char *fn)
{
    int i = 0;
    int n_slashes = 0;
    int len = 0;
    if (strlen(fn) != 0) {
        len = strlen(fn);
    }
    if (len != 0) {
        for (; i < len; ++i)
            if (fn[i] == '/') ++n_slashes;
    }
    return n_slashes;
}

int insertToMenu(char *str, char **out)
{
  int len_str = strlen(str);
  copy(out, str, len_str);
  return 1;
}

char *insert_to_menu(char *str)
{
    int len_str = strlen(str);
    char *add = (char *)malloc(sizeof(char) * (len_str + 1));
    if (add) {
        strcpy(add, str);
        add[len_str] = '\0';
        return add;
    }
    return "error malloc";
}

int get_last_slash_pos(char *name)
{
    int j = strlen(name) - 1;
    for (; j >= 0; --j)
        if (name[j] == '/') return j;
    return -1;
}

int get_name2(char *fn, char **out)
{
  int len = strlen(fn);
  int s_pos = get_last_slash_pos(fn);
  if (s_pos == -1) {
    fprintf(stderr, "s_pos: %d\n", s_pos);
    exit(-1);
  }
  char *s_path = NULL;
  if (s_pos >= 0) {
    copy(out, fn + s_pos + 1, len - s_pos);
/*
    s_path = (char *)malloc(sizeof(char) * (len - s_pos + 1));
    if (s_path) {
      strncpy(s_path, fn + s_pos + 1, len - s_pos);
      s_path[len - s_pos] = '\0';
    }
*/
  }
  return 1;
}

char *get_name(char *fn)
{
    int len = strlen(fn);
    int s_pos = get_last_slash_pos(fn);
    if (s_pos == -1) {
        fprintf(stderr, "s_pos: %d\n", s_pos);
        exit(-1);
    }
    char *s_path = NULL;
    if (s_pos >= 0) {
        s_path = (char *)malloc(sizeof(char) * (len - s_pos + 1));
        if (s_path) {
            strncpy(s_path, fn + s_pos + 1, len - s_pos);
            s_path[len - s_pos] = '\0';
        }
    }
    return s_path;
}

char **get_names_only(Array *a)
{
    char **names = (char **)calloc(a->n_elements, sizeof(char *));
    int i;
    int len = 0;
    int slash_pos = 0;
    int total = 0;
    for (i = 0; i < a->n_elements; ++i) {
        slash_pos = get_last_slash_pos(a->menu[i].name);
        if (slash_pos >= 0) {
            len = strlen(a->menu[i].name);
            total = len - slash_pos;
            names[i] = (char *)malloc(sizeof(char) * (total + 1));
            strncpy(names[i], &a->menu[i].name[slash_pos + 1], total);
            names[i][total] = '\0';
        }
    }
    return names;
}

int getParent(char *child, char **parent_out)
{
  size_t len_child = strlen(child);
  size_t i, counter = 0;
  if (len_child > 0) {
    for (i = len_child - 1; i >= 0; --i) if (child[i] == '/' && ++counter == 2) break;
    copy(parent_out, child, i);
    return 1;
  }
  return 0;
}

char *get_parent(char *fn)
{
    int pos_slash = get_last_slash_pos(fn);
    char *parent = NULL;
    // il faut mettre pos_slash + 1 sinon segfault si on met pos_slash seulement
    parent = (char *)malloc(sizeof(char) * (pos_slash + 1));
    char *real_parent = NULL;
    if (parent) {
        strncpy(parent, fn, pos_slash);
        parent[pos_slash] = '\0';
        int second_slash = get_last_slash_pos(parent);
        real_parent = (char *)malloc(sizeof(char) * (second_slash + 1));
        if (real_parent) {
            strncpy(real_parent, parent, second_slash);
            real_parent[second_slash] = '\0';
        }
        free(parent);
        parent = NULL;
    }
    //fprintf(stderr, "parent: %s\n", parent);
    /*
    if (strlen(fn) != 1 && fn[0] != '/') {
        fn[get_last_slash_pos(fn)] = '\0';
    }

    return fn;
    */
    //return parent;
    return real_parent;
}

int free_menu(Menu *menu)
{
  if (menu->name != NULL) {
    free(menu->name);
    menu->name = NULL;
  }
  if (menu->type != NULL) {
    free(menu->type);
    menu->type = NULL;
  }
  if (menu->complete_path != NULL) {
    free(menu->complete_path);
    menu->complete_path = NULL;
  }
/*
  if (menu->parent != NULL) {
    free(menu->parent);
    menu->parent = NULL;
  }
*/
  return 1;
}

#define num_info "info.st_mode: %d"
#define err_str  "Error: %d, %s."
#define LEN_FN "fn length: %d"
void parcours(char *fn, int indent, Array *a, int recursive, Window *w)
{
  DIR *dir;
  struct dirent *entry;
  struct stat info;
  char path[MAXPATHLEN];
  Menu menu = {};
  /*
     menu.name = NULL;
     menu.type = NULL;
     menu.complete_path = NULL;
     menu.parent = NULL;
     */
  char *path_name = NULL;

  int len = strlen(fn);
  int is_root = 0;

  if (len == 1 && fn[0] == '/') {
    is_root = 1;
  } else if (len > 1) {
    is_root = 0;
  }

  char del_in[IN_SZ];
  sprintf(del_in, del, 40);
  char m_place[PLACE_SZ];
  char error_str[sizeof(err_str)];
  char *str = strerror(errno);
  char *open_err = "opendir error.";

  int counter = 0;

  if ((dir = opendir(fn)) == NULL) {
    sprintf(m_place, place, w->y_size - 2, w->x_beg);
    move(1, m_place);
    write(1, del_line, sizeof(del_line));
    write(1, open_err, strlen(open_err));
    sprintf(m_place, place, w->y_size - 1, w->x_beg);
    move(1, m_place);
    write(1, del_line, sizeof(del_line));
    write(1, fn, strlen(fn));
    char lenfn[sizeof(LEN_FN)];
    int length_fn = strlen(fn);
    sprintf(lenfn, LEN_FN, length_fn);
    write(1, lenfn, strlen(lenfn));
  } else {
    while ((entry = readdir(dir)) != NULL) {
      if (entry->d_name[0] != '.') {
        strcpy(path, fn);
        if (!is_root) {
          strcat(path, "/");
        }
        strcat(path, entry->d_name);
        //path_name = get_name(path);
        get_name2(path, &path_name);
        // if (path_name != NULL)
        /*
           menu.name = insert_to_menu(path_name);
           menu.complete_path = insert_to_menu(path);
           */
        insertToMenu(path_name, &(menu.name));
        insertToMenu(path, &(menu.complete_path));

        if (num_of_slashes(path) > 1 && counter == 0) { // && recursive == 0
          /*
             menu.parent = get_parent(path);

*/
          getParent(path, &(menu.parent));
          ++counter;
        }

        if (debug_mode) {
          // a partir d'un autre thread renvoyer a un autre tty

          if (menu.parent != NULL) {
            sprintf(m_place, place, w->y_size, w->x_beg + 42);
            move(1, m_place);
            sprintf(del_in, del, 40);
            del_from_cursor(del_in);
            write(1, "menu.parent: ", strlen("menu.parent: "));
            write(1, menu.parent, strlen(menu.parent));
          }

          sprintf(m_place, place, w->y_size - 5, w->x_beg + 2);
          move(1, m_place);
          write(1, del_line, sizeof(del_line));
          write(1, "menu.name: ", strlen("menu.name: "));
          write(1, menu.name, strlen(menu.name));

          sprintf(m_place, place, w->y_size - 4, w->x_beg + 2);
          move(1, m_place);
          write(1, del_line, sizeof(del_line));
          write(1, "menu.complete_path: ", strlen("menu.complete_path: "));
          write(1, menu.complete_path, strlen(menu.complete_path));

          sprintf(m_place, place, w->y_size - 3, w->x_beg + 2);
          move(1, m_place);
          write(1, del_line, sizeof(del_line));
          write(1, "path_name: ", strlen("path_name: "));
          write(1, path_name, strlen(path_name));

          sprintf(m_place, place, w->y_size - 2, w->x_beg + 2);
          move(1, m_place);
          write(1, del_line, sizeof(del_line));
          write(1, "path: ", strlen("path: "));
          write(1, path, strlen(path));

          sprintf(m_place, place, w->y_size - 1, w->x_beg + 2);
          move(1, m_place);
          write(1, del_line, sizeof(del_line));
          char num[sizeof(num_info)];
          sprintf(num, num_info, info.st_mode);
          write(1, num, sizeof(num));
        }

        //if (menu.parent != NULL && counter < 1) {
        if (menu.parent != NULL /* && counter < 1 */) {
          free(menu.parent);
          menu.parent = NULL;
        }
        if (stat(path, &info) != 0) {
          //fprintf(stderr, "Error: %d, %s.\n", errno, strerror(errno));
          sprintf(error_str, err_str, errno, str);
          sprintf(m_place, place, w->y_size - 1, w->x_beg + 2);
          move(1, m_place);
          write(1, del_line, sizeof(del_line));
          write(1, error_str, strlen(error_str));
        } else if (S_ISLNK(info.st_mode)) {
          //menu.type = insert_to_menu("symbolic_link");
          char *symb = "symbolic_link";
          insertToMenu(symb, &menu.type);
        } else if (S_ISREG(info.st_mode)) {
          char *file_ = "file";
          insertToMenu(file_, &menu.type);
          //  menu.type = insert_to_menu("file");
        }
        /*else if (entry->d_type == DT_UNKNOWN) {
          menu.type = insert_to_menu("unknown");
          }
          */
        else {
          char *unknown_ = "unknown";
          insertToMenu(unknown_, &menu.type);
          //menu.type = insert_to_menu("unknown");
        }
        if (S_ISDIR(info.st_mode)) {
          char *directory_ = "directory";
          insertToMenu(directory_, &menu.type);
          //menu.type = insert_to_menu("directory");
          if (recursive) {
            if (path_name != NULL) {
              free(path_name);
              path_name = NULL;
            }
            free_menu(&menu);
            parcours(path, indent + 1, a, 1, w);
          }
        }
        if (debug_mode) {
          sprintf(m_place, place, w->y_size, w->x_beg + 2);
          move(1, m_place);
          sprintf(del_in, del, 40);
          del_from_cursor(del_in);
          write(1, "menu.type: ", strlen("menu.type: "));
          write(1, menu.type, strlen(menu.type));
        }
        /*
           add_menu(a, menu);
           */
        //addMenu(&a, menu);
        addMenu2(&a, &menu);
        if (menu.name != NULL) {
          free(menu.name);
          menu.name = NULL;
        }
        if (menu.type != NULL) {
          free(menu.type);
          menu.type = NULL;
        }
        if (menu.complete_path != NULL) {
          free(menu.complete_path);
          menu.complete_path = NULL;
        }
        if (path_name != NULL) {
          free(path_name);
          path_name = NULL;
        }
/*
        if (menu.parent != NULL && counter < 1) {
          free(menu.parent);
          menu.parent = NULL;
        }
*/

        if (debug_mode) {
          //exit(-1);
        }
      }
    }
    (void)closedir(dir);
  }
}
