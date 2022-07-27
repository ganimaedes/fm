#include "array.h"

void initialize_array2(Array ***a, int initial_size)
{
  *(*a) = calloc(1, sizeof *(*(*a)));
  if (*(*a) == NULL) {
    PRINT("Error calloc");
  }
  (*(*a))->menu = (Menu *)calloc(initial_size, sizeof *(*(*a))->menu);
  if ((*(*a))->menu == NULL) {
    PRINT("Error calloc");
  }
  (*(*a))->n_elements = 0;
  (*(*a))->capacity = initial_size;
}

void initialize_array(Array **a, int initial_size)
{
  *a = calloc(1, sizeof *(*a));
  if (*a == NULL) {
    PRINT("Error calloc");
  }
  (*a)->menu = (Menu *)calloc(initial_size, sizeof *(*a)->menu);
  if ((*a)->menu == NULL) {
    PRINT("Error calloc");
  }
  (*a)->n_elements = 0;
  (*a)->capacity = initial_size;
}

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

void addMenu2(Array **a, Menu *menu)
{
  if ((*a)->n_elements >= (*a)->capacity) {
    double_capacity(*a);
  }
  int len = strlen(menu->name);
  copy(&((*a)->menu[(*a)->n_elements].name), menu->name, len);

  (*a)->menu[(*a)->n_elements].type = menu->type;

  if (menu->complete_path != NULL) {
    len = strlen(menu->complete_path);
    copy(&((*a)->menu[(*a)->n_elements].complete_path), menu->complete_path, len);
  }

  if (menu->parent != NULL) {
    len = strlen(menu->parent);
    copy(&((*a)->menu[(*a)->n_elements].parent), menu->parent, len);
  }

  if (menu->permissions != NULL) {
    len = strlen(menu->permissions);
    copy(&((*a)->menu[(*a)->n_elements].permissions), menu->permissions, len);
  }

  ++(*a)->n_elements;
}

void free_array3(Array ***a)
{
  for (int i = 0; i < (*(*a))->n_elements; ++i) {
    if ((*(*a))->menu[i].name != NULL) {
      free((*(*a))->menu[i].name);
      (*(*a))->menu[i].name = NULL;
    }
    if ((*(*a))->menu[i].complete_path != NULL) {
      free((*(*a))->menu[i].complete_path);
      (*(*a))->menu[i].complete_path = NULL;
    }
    if ((*(*a))->menu[i].permissions != NULL) {
      free((*(*a))->menu[i].permissions);
      (*(*a))->menu[i].permissions = NULL;
    }
  }
  if ((*(*a))->menu != NULL) {
    free((*(*a))->menu);
    (*(*a))->menu = NULL;
  }
  if (*(*a) != NULL) {
    free(*(*a));
    *(*a) = NULL;
  }
}

void free_array2(Array **a)
{
  for (int i = 0; i < (*a)->n_elements; ++i) {
    if ((*a)->menu[i].name != NULL) {
      free((*a)->menu[i].name);
      (*a)->menu[i].name = NULL;
    }
    if ((*a)->menu[i].complete_path != NULL) {
      free((*a)->menu[i].complete_path);
      (*a)->menu[i].complete_path = NULL;
    }
    if ((*a)->menu[i].permissions != NULL) {
      free((*a)->menu[i].permissions);
      (*a)->menu[i].permissions = NULL;
    }
    if ((*a)->menu[i].parent != NULL) {
      free((*a)->menu[i].parent);
      (*a)->menu[i].parent = NULL;
    }
  }
  if ((*a)->menu != NULL) {
    free((*a)->menu);
    (*a)->menu = NULL;
  }
  if (*a != NULL) {
    free(*a);
    *a = NULL;
  }
}

void free_array(Array *a)
{
  for (int i = 0; i < a->n_elements; ++i) {
    if (a->menu[i].name != NULL) {
      free(a->menu[i].name);
      a->menu[i].name = NULL;
    }
    if (a->menu[i].complete_path != NULL) {
      free(a->menu[i].complete_path);
      a->menu[i].complete_path = NULL;
    }
    if (a->menu[i].permissions != NULL) {
      free(a->menu[i].permissions);
      a->menu[i].permissions = NULL;
    }
    /*
    if (a->menu[i].parent != NULL) {
      free(a->menu[i].parent);
      a->menu[i].parent = NULL;
    }
    */
  }
  if (a->menu != NULL) {
    free(a->menu);
    a->menu = NULL;
  }
  if (a != NULL) {
    free(a);
    a = NULL;
  }
}

void print_array(Array *a)
{
  for (int i = 0; i < a->n_elements; ++i) {
    printf("%s : %s\n", a->menu[i].name, a->menu[i].type);
  }
}

void dupArray2(Array *in, Array *out)
{
  int i;
  int len = 0;
  Menu menu = { 0 };
  for (i = 0; i < in->n_elements; ++i) {
    len = strlen(in->menu[i].name);
    copy(&(menu.name), in->menu[i].name, len);
    menu.type = in->menu[i].type;
    len = strlen(in->menu[i].complete_path);
    copy(&(menu.complete_path), in->menu[i].complete_path, len);
    len = strlen(in->menu[i].permissions);
    copy(&(menu.permissions), in->menu[i].permissions, len);
    /*
    if (in->menu[i].parent != NULL && (len = strlen(in->menu[i].parent)) > 0) {
      copy(&(menu.parent), in->menu[i].parent, len);
    }
    */
    addMenu2(&out, &menu);
/*
    if (in->menu[i].has_scroll) {
      out->menu[i].upper_pos = in->menu[i].upper_pos;
      out->menu[i].lower_pos = in->menu[i].lower_pos;
      out->menu[i].highlighted_pos = in->menu[i].highlighted_pos;
      out->menu[i].previous_pos = in->menu[i].previous_pos;
    }
*/
    if (menu.name) {
      free(menu.name);
      menu.name = NULL;
    }
    if (menu.complete_path) {
      free(menu.complete_path);
      menu.complete_path = NULL;
    }
    if (menu.permissions) {
      free(menu.permissions);
      menu.permissions = NULL;
    }
  }
}
