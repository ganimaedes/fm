#ifndef ARRAY_H
#define ARRAY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int size = 2;

static char *entries[2] = { "entry1", "entry2" };
static char *types[2] = { "directory", "file" };

typedef struct {
    char *name;
    char *type;
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

#endif // ARRAY_H
