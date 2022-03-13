#include "positions.h"

void init_attr(Attributes *attr, int size)
{
  attr->pos = (Positions **)calloc(size, sizeof(Positions*));
  attr->paths = (char **)calloc(size, sizeof(char *));
  if (attr->pos && attr->paths) {
    attr->capacity = size;
    attr->n_elements = 0;
  }
}

void double_cap(Attributes *attr)
{
  attr->capacity *= 2;
  if (attr->capacity == 0)
    attr->capacity = 1;

  void *tmp = (Positions **)realloc(attr->pos, attr->capacity * sizeof(Positions *));
  if (tmp == NULL) {
    free_attr(attr);
    fprintf(stderr, "Error: Positions realloc\n");
    exit(-1);
  }
  attr->pos = tmp;

  void *temp = (char **)realloc(attr->paths, attr->capacity * sizeof(char *));
  if (temp == NULL) {
    free_attr(attr);
    fprintf(stderr, "Error: Paths realloc\n");
    exit(-1);
  }
  attr->paths = temp;
}

void add_attr(Attributes *attr, Positions *positions, char *path)
{
  if (attr->n_elements >= attr->capacity)
    double_cap(attr);

  if ((attr->pos[attr->n_elements] = (Positions *)malloc(sizeof(Positions))) != NULL) {
    attr->pos[attr->n_elements]->m_position = positions->m_position;
    attr->pos[attr->n_elements]->m_upper_pos = positions->m_upper_pos;
    attr->pos[attr->n_elements]->m_lower_pos = positions->m_lower_pos;
  }

  int len = strlen(path);
  if ((attr->paths[attr->n_elements] = (char *)malloc(sizeof(char) * (len + 1))) != NULL) {
    strncpy(attr->paths[attr->n_elements], path, len);
    attr->paths[attr->n_elements][len] = '\0';
  }
  ++attr->n_elements;
}

void free_attr(Attributes *attr)
{
  int i;
  for (i = 0; i < attr->n_elements; ++i) {
    free(attr->pos[i]);
    attr->pos[i] = NULL;
    free(attr->paths[i]);
    attr->paths[i] = NULL;
  }
  free(attr->pos);
  attr->pos = NULL;
  free(attr->paths);
  attr->paths = NULL;
}
