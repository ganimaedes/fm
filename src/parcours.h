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

// Turn A into a string literal without expanding macro definitions
// (however, if invoked from a macro, macro arguments are expanded).
#define STRINGIZE_NX(A) #A
// Turn A into a string literal after macro-expanding it.
#define STRINGIZE(A) STRINGIZE_NX(A)

//static int file_descriptor = 1;
#ifndef __file_descriptor
#define __file_descriptor 1
#endif // __file_descriptor
#ifndef write_line_debug
#define write_line_debug(fd, str) if (write((fd), (str), strlen(str)) < 0) { fprintf(stderr, "Error write\n"); exit(1); }
#endif // write_line_debug

#ifndef PRINTSTRINGSAMELINE
#define PRINTSTRINGSAMELINE(value) do {                     \
  write_line_debug(__file_descriptor, STRINGIZE_NX(value)); \
  write_line_debug(__file_descriptor, ": ");                \
  write_line_debug(__file_descriptor, (value));             \
} while(0)
#endif // PRINTSTRINGSAMELINE

static int debug_mode = 0;

int get_permissions2(char *path, char permissions[], struct stat *fileStat);
//int num_of_slashes(char *fn);
unsigned int num_of_slashes(const char *fn);
int get_last_slash_pos(char *name);
//char *get_parent(char *fn);
//int get_parent(char *child, char **parent_out, char *type);
int get_parent(char *child, char **parent_out, int max);
int free_menu(Menu *menu);
int getParent(char *child, char **parent_out);
int insertToMenu(char *str, char **out);
void parcours(char *fn, int indent, Array *a, int recursive, Window_ *w);
void remove_files_from_folder(char *fn, Window_ *w);
int check_if_file_exists(char *fn, char *file_name, Window_ *w);

#endif  // PARCOURS_H
