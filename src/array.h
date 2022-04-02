#ifndef ARRAY_H
#define ARRAY_H

#include "scr.h"

#if !defined(ARRAY_SIZE)
    #define ARRAY_SIZE(x) (sizeof((x)) / sizeof((x)[0]))
#endif

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

static char *TYPE[] = {
  "j", /* JPEG JIFF */
  "p", /* PNG       */
  "g", /* GIF       */
  "d", /* PDF       */
  "o"  /* FOLDER    */
};

typedef struct _STAT_INFO {
  unsigned short width;
  unsigned short height;
  unsigned int file_len;
  char *file_name;
  FILE *file;
  unsigned char *data;
} STAT_INFO;

typedef struct {
    char *name;
    char *type;
    char *complete_path;
    char *parent;
    int parent_pos;
    char *permissions;
    //int displaced_entry;
    //int upper_pos;
    //int lower_pos;
    //int pos;
} Menu;

typedef struct {
    Menu *menu;
    int n_elements;
    int capacity;
} Array;

void init(Array *a, int initial_size);
void double_capacity(Array *a);
void add_menu(Array *a, Menu menu);
void addMenu(Array **a, Menu menu);
void addMenu2(Array **a, Menu *menu);
void free_array(Array *a);
void print_array(Array *a);
void dup_array(Array *in, Array *out);
void dupArray2(Array *in, Array *out);

#endif // ARRAY_H
