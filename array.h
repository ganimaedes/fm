#ifndef ARRAY_H
#define ARRAY_H

#include "scr.h"

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

typedef struct {
    char *name;
    char *type;
    char *complete_path;
    char *parent;
    int parent_pos;
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
void free_array(Array *a);
void print_array(Array *a);
void dup_array(Array *in, Array *out);

#endif // ARRAY_H
