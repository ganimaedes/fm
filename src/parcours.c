#include "parcours.h"
#include "scr.h"
#include "copy.h"

int get_permissions2(char *path, char permissions[], struct stat *fileStat)
{
  //printf("File Size: \t\t%lu bytes\n", fileStat.st_size);
  //printf("Number of Links: \t%lu\n", fileStat.st_nlink);
  //printf("File inode: \t\t%lu\n", fileStat.st_ino);

  char value;
  value = (S_ISDIR(fileStat->st_mode)) ? 'd' : '-';
  memcpy(&permissions[0], &value, 1);
  value = (fileStat->st_mode & S_IRUSR) ? 'r' : '-';
  memcpy(&permissions[1], &value, 1);
  value = (fileStat->st_mode & S_IWUSR) ? 'w' : '-';
  memcpy(&permissions[2], &value, 1);
  value = (fileStat->st_mode & S_IXUSR) ? 'x' : '-';
  memcpy(&permissions[3], &value, 1);
  value = (fileStat->st_mode & S_IRGRP) ? 'r' : '-';
  memcpy(&permissions[4], &value, 1);
  value = (fileStat->st_mode & S_IWGRP) ? 'w' : '-';
  memcpy(&permissions[5], &value, 1);
  value = (fileStat->st_mode & S_IXGRP) ? 'x' : '-';
  memcpy(&permissions[6], &value, 1);
  value = (fileStat->st_mode & S_IROTH) ? 'r' : '-';
  memcpy(&permissions[7], &value, 1);
  value = (fileStat->st_mode & S_IWOTH) ? 'w' : '-';
  memcpy(&permissions[8], &value, 1);
  value = (fileStat->st_mode & S_IXOTH) ? 'x' : '-';
  memcpy(&permissions[9], &value, 1);
  permissions[10] = '\0';

  //printf("The %s %s a symbolic link\n", (S_ISDIR(fileStat.st_mode)) ? "folder" : "file", (S_ISLNK(fileStat.st_mode)) ? "is" : "is not");
  return 1;
}

unsigned int num_of_slashes(const char *fn)
{
  unsigned int i = 0;
  unsigned int n_slashes = 0;
  for (; *(fn + i) != '\0'; ++i)
    if (*(fn + i) == '/')
      ++n_slashes;
  return n_slashes;
}

int insertToMenu(char *str, char **out)
{
  int len_str = strlen(str);
  copy(out, str, len_str);
  return 1;
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
  }
  return 1;
}

//int get_parent(char *child, char **parent_out, char *type)
int get_parent(char *child, char **parent_out, int max)
{
  size_t len_child = strlen(child);
  size_t i, counter = 0;
         //n_for_type = (*type == 'd') ? 2 : 3;
         //n_for_type = 2;
         //n_for_type = 1;
  if (len_child > 0) {
    //for (i = len_child - 1; i >= 0; --i) if (child[i] == '/' && ++counter == n_for_type) break;
    for (i = len_child - 1; i >= 0; --i) if (child[i] == '/' && ++counter == max) break;
    copy(parent_out, child, i);
    return 1;
  }
  return 0;
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

int free_menu(Menu *menu)
{
  if (menu->name != NULL) {
    free(menu->name);
    menu->name = NULL;
  }
  if (menu->complete_path != NULL) {
    free(menu->complete_path);
    menu->complete_path = NULL;
  }
  if (menu->parent != NULL) {
    free(menu->parent);
    menu->parent = NULL;
  }
  if (menu->permissions != NULL) {
    free(menu->permissions);
    menu->permissions = NULL;
  }
  return 1;
}

#define num_info "info.st_mode: %d"
#define err_str  "Error: %d, %s."
#define LEN_FN "fn length: %d"
void parcours(char *fn, int indent, Array *a, int recursive, Window_ *w)
{
  DIR *dir;
  struct dirent *entry;
  struct stat info;
  char path[MAXPATHLEN];
  Menu menu = {};

  int len = strlen(fn);
  int is_root = 0;

  if (len == 1 && fn[0] == '/') {
    is_root = 1;
  } else if (len > 1) {
    is_root = 0;
  }

  char del_in[IN_SZ];
  sprintf(del_in, del, 40);
  //char m_place[PLACE_SZ];
  char m_place[sizeof(place_) + 27];
  char error_str[sizeof(err_str)];
  char *str = strerror(errno);
  char *open_err = "opendir error.";

  int counter = 0;

  if ((dir = opendir(fn)) == NULL) {
    sprintf(m_place, place_, w->y_size - 2, w->x_beg);
    move(1, m_place);
    write(1, del_line, sizeof(del_line));
    write(1, open_err, strlen(open_err));
    sprintf(m_place, place_, w->y_size - 1, w->x_beg);
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
        insertToMenu(entry->d_name, &(menu.name));
        insertToMenu(path, &(menu.complete_path));

        if (num_of_slashes(path) > 1 && counter == 0) { // && recursive == 0
          getParent(path, &(menu.parent));
          ++counter;
        }

        if (debug_mode) {
          // a partir d'un autre thread renvoyer a un autre tty

          if (menu.parent != NULL) {
            sprintf(m_place, place_, w->y_size, w->x_beg + 42);
            move(1, m_place);
            sprintf(del_in, del, 40);
            del_from_cursor(del_in);
            PRINTSTRINGSAMELINE(menu.parent);
          }

          sprintf(m_place, place_, w->y_size - 5, w->x_beg + 2);
          move(1, m_place);
          write(1, del_line, sizeof(del_line));
          PRINTSTRINGSAMELINE(menu.name);

          sprintf(m_place, place_, w->y_size - 4, w->x_beg + 2);
          move(1, m_place);
          write(1, del_line, sizeof(del_line));
          PRINTSTRINGSAMELINE(menu.complete_path);

          sprintf(m_place, place_, w->y_size - 3, w->x_beg + 2);
          move(1, m_place);
          write(1, del_line, sizeof(del_line));
          PRINTSTRINGSAMELINE(entry->d_name);

          sprintf(m_place, place_, w->y_size - 2, w->x_beg + 2);
          move(1, m_place);
          write(1, del_line, sizeof(del_line));
          PRINTSTRINGSAMELINE(path);

          sprintf(m_place, place_, w->y_size - 1, w->x_beg + 2);
          move(1, m_place);
          write(1, del_line, sizeof(del_line));
          char num[sizeof(num_info)];
          sprintf(num, num_info, info.st_mode);
          write(1, num, sizeof(num));
        }

        ///*
        if (menu.parent != NULL) {
          free(menu.parent);
          menu.parent = NULL;
        }
        //*/
        if (stat(path, &info) != 0) {
          sprintf(error_str, err_str, errno, str);
          sprintf(m_place, place_, w->y_size - 1, w->x_beg + 2);
          move(1, m_place);
          write(1, del_line, sizeof(del_line));
          write(1, error_str, strlen(error_str));
        } else if (S_ISLNK(info.st_mode)) {
          char *symb = "s";
          menu.type = symb;
        } else if (S_ISREG(info.st_mode)) {
          char *file_ = "f";
          menu.type = file_;
        }
        /*else if (entry->d_type == DT_UNKNOWN) {
          }
          */
/*
        else {
          char *unknown_ = "unknown";
          insertToMenu(unknown_, &menu.type);
        }
*/
        if (S_ISDIR(info.st_mode)) {
          char *directory_ = "d";
          menu.type = directory_;
          if (recursive) {
            free_menu(&menu);
            parcours(path, indent + 1, a, 1, w);
          }
        }
        if (debug_mode) {
          sprintf(m_place, place_, w->y_size, w->x_beg + 2);
          move(1, m_place);
          sprintf(del_in, del, 40);
          del_from_cursor(del_in);
          PRINTSTRINGSAMELINE(menu.type);
        }
        char perm[11];
        get_permissions2(menu.complete_path, perm, &info);
        menu.permissions = malloc(11 * sizeof *menu.permissions);
        if (menu.permissions == NULL) {
          PRINT("malloc");
        }
        memcpy(menu.permissions, perm, 10);
        menu.permissions[10] = '\0';
        addMenu2(&a, &menu);
        free_menu(&menu);

        if (debug_mode) {
          //exit(-1);
        }

      }
    }
    (void)closedir(dir);
  }
}

void remove_files_from_folder(char *fn, Window_ *w)
{
  DIR *dir;
  struct dirent *entry;
  struct stat info;
  char path[MAXPATHLEN];

  char del_in[IN_SZ];
  sprintf(del_in, del, 40);
  char m_place[sizeof(place_) + 27];
  int len = strlen(fn);
  int is_root = 0;
  char *str = strerror(errno);

  if (len == 1 && fn[0] == '/') {
    is_root = 1;
  } else if (len > 1) {
    is_root = 0;
  }

  char error_str[sizeof(err_str)];
  char *open_err = "opendir error.";

  if ((dir = opendir(fn)) == NULL) {
    sprintf(m_place, place_, w->y_size - 2, w->x_beg);
    move(1, m_place);
    write(1, del_line, sizeof(del_line));
    write(1, open_err, strlen(open_err));
    sprintf(m_place, place_, w->y_size - 1, w->x_beg);
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
        strcat(path, "/");
        strcat(path, entry->d_name);

        if (stat(path, &info) != 0) {
          sprintf(error_str, err_str, errno, str);
          sprintf(m_place, place_, w->y_size - 1, w->x_beg + 2);
          move(1, m_place);
          write(1, del_line, sizeof(del_line));
          write(1, error_str, strlen(error_str));
        } else if (S_ISREG(info.st_mode)) {
          unlink(path);
        }
        if (S_ISDIR(info.st_mode)) {
          remove(path);
        }
      }
    }
    (void)closedir(dir);
  }
}

int check_if_file_exists(char *fn, char *file_name, Window_ *w)
{
  DIR *dir;
  struct dirent *entry;
  struct stat info;
  char path[MAXPATHLEN];

  char del_in[IN_SZ];
  sprintf(del_in, del, 40);
  char m_place[sizeof(place_) + 27];
  int len = strlen(fn);
  int is_root = 0;
  char *str = strerror(errno);

  if (len == 1 && fn[0] == '/') {
    is_root = 1;
  } else if (len > 1) {
    is_root = 0;
  }
  int len_file_name = strlen(file_name);

  char error_str[sizeof(err_str)];
  char *open_err = "opendir error.";

  char *path_ptr;

  if ((dir = opendir(fn)) == NULL) {
    sprintf(m_place, place_, w->y_size - 2, w->x_beg);
    move(1, m_place);
    write(1, del_line, sizeof(del_line));
    write(1, open_err, strlen(open_err));
    sprintf(m_place, place_, w->y_size - 1, w->x_beg);
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
        //strcat(path, "/");
        strcat(path, entry->d_name);

        if (stat(path, &info) != 0) {
          sprintf(error_str, err_str, errno, str);
          sprintf(m_place, place_, w->y_size - 1, w->x_beg + 2);
          move(1, m_place);
          write(1, del_line, sizeof(del_line));
          write(1, error_str, strlen(error_str));
        } else if (S_ISREG(info.st_mode)) {
          path_ptr = &path[0];
          if (strncmp(file_name, path_ptr, len_file_name) == 0) {
            return 1;
          }
        }
        if (S_ISDIR(info.st_mode)) {
          check_if_file_exists(path, file_name, w);
        }
      }
    }
    (void)closedir(dir);
  }
  return 0;
}
