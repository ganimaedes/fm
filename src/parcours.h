#ifndef PARCOURS_H
#define PARCOURS_H

#include "array.h"

#include <dirent.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/stat.h>

//#include <sys/types.h>

//static int maxlen = 40;
//static int is_root = 0;

static int debug_mode = 0;

int num_of_slashes(char *fn);
char *insert_to_menu(char *str);
int get_last_slash_pos(char *name);
char *get_name(char *fn);
char **get_names_only(Array *a);
char *get_parent(char *fn);
int free_menu(Menu *menu);
int getParent(char *child, char **parent_out);
int insertToMenu(char *str, char **out);
void parcours(char *fn, int indent, Array *a, int recursive, Window *w);

#endif  // PARCOURS_H
