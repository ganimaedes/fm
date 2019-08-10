#include "array.h"

void init(Array *a, int initial_size)
{
    a->menu = (Menu *)calloc(initial_size, sizeof(Menu));
    a->n_elements = 0;
    a->capacity = initial_size;
}

void double_capacity(Array *a)
{
    a->capacity *= 2;
    if (a->capacity == 0) {
        a->capacity = 1;
    }
    a->menu = (Menu *)realloc(a->menu, a->capacity * sizeof(Menu));
}

void add_menu(Array *a, Menu menu)
{
    if (a->n_elements >= a->capacity) {
        double_capacity(a);
    }
    a->menu[a->n_elements].name = (char *)malloc(sizeof(char) * (strlen(menu.name) + 1));
    strcpy(a->menu[a->n_elements].name, menu.name);
    
    a->menu[a->n_elements].type = (char *)malloc(sizeof(char) * (strlen(menu.type) + 1));
    strcpy(a->menu[a->n_elements].type, menu.type);
    ++a->n_elements;
}

void free_array(Array *a)
{
    for (int i = 0; i < a->n_elements; ++i) {
        free(a->menu[i].name);
        free(a->menu[i].type);
    }
    free(a->menu);
    a->menu = NULL;
}

void print_array(Array *a)
{
    for (int i = 0; i < a->n_elements; ++i) {
        printf("%s : %s\n", a->menu[i].name, a->menu[i].type);
    }
}
