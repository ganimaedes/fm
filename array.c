#include "array.h"

void init(Array *a, int initial_size)
{
    a->menu = (Menu *)calloc(initial_size, sizeof(Menu));
    if (a->menu) {
        a->n_elements = 0;
        a->capacity = initial_size;
    }
}

void double_capacity(Array *a)
{
    a->capacity *= 2;
    if (a->capacity == 0) {
        a->capacity = 1;
    }
    void *b = (Menu *)realloc(a->menu, a->capacity * sizeof(Menu));
    if (b == NULL) {
        free_array(a);
        restore_config;
        fprintf(stderr, "Error: realloc\n");
        exit(1);
    }
    a->menu = b;
}

void add_menu(Array *a, Menu menu)
{
    if (a->n_elements >= a->capacity) {
        double_capacity(a);
    }
    int len = strlen(menu.name);
    a->menu[a->n_elements].name = (char *)malloc(sizeof(char) * (len + 1));
    if (a->menu[a->n_elements].name) {
        strcpy(a->menu[a->n_elements].name, menu.name);
        a->menu[a->n_elements].name[len] = '\0';
    }
    
    len = strlen(menu.type);
    a->menu[a->n_elements].type = (char *)malloc(sizeof(char) * (len + 1));
    if (a->menu[a->n_elements].type) {
        strcpy(a->menu[a->n_elements].type, menu.type);
        a->menu[a->n_elements].type[len] = '\0';
    }
    
    len = strlen(menu.complete_path);
    a->menu[a->n_elements].complete_path = (char *)malloc(sizeof(char) * (len + 1));
    if (a->menu[a->n_elements].complete_path) {
        strcpy(a->menu[a->n_elements].complete_path, menu.complete_path);
        a->menu[a->n_elements].complete_path[len] = '\0';
    }
    ++a->n_elements;
}

void free_array(Array *a)
{
    for (int i = 0; i < a->n_elements; ++i) {
        free(a->menu[i].name);
        free(a->menu[i].type);
        free(a->menu[i].complete_path);
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

void dup_array(Array *in, Array *out)
{
    int i;
    int len = 0;
    Menu menu;
    for (i = 0; i < in->n_elements; ++i) {
        len = strlen(in->menu[i].name);
        menu.name = (char *)malloc(sizeof(char) * (len + 1));
        if (menu.name) {
            strcpy(menu.name, in->menu[i].name);
            menu.name[len] = '\0';
        }
        len = strlen(in->menu[i].type);
        menu.type = (char *)malloc(sizeof(char) * (len + 1));
        if (menu.type) {
            strcpy(menu.type, in->menu[i].type);
            menu.type[len] = '\0';
        }
        len = strlen(in->menu[i].complete_path);
        menu.complete_path = (char *)malloc(sizeof(char) * (len + 1));
        if (menu.complete_path) {
            strcpy(menu.complete_path, in->menu[i].complete_path);
            menu.complete_path[len] = '\0';
        }
        add_menu(out, menu);
        free(menu.name);
        free(menu.type);
        free(menu.complete_path);
    }
}
