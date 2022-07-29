#ifndef POSITIONS_H
#define POSITIONS_H

#include "scr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int m_position;
    int m_upper_pos;
    int m_lower_pos;
    int window_number;
    int array_size;
    //int alpha_pos;
    int *alpha_pos;
    int num_alpha_pos;
} Positions;

typedef struct {
    char **paths;
    Positions **pos;
    int *alpha_pos;
    int counter;
    int n_elements;
    int capacity;
} Attributes;

void add_to_positions(Positions **positions, int *alpha_positions, int n_positions);
void free_positions(Positions *positions);
void duplicate_attributes(Attributes *attr_in, Attributes **attr_out);
void initialize_attr(Attributes **attr, int size);
void init_attr(Attributes *attr, int size);
void double_cap(Attributes *attr);
void add_attr(Attributes *attr, Positions *positions, char *path);
void free_attr(Attributes *attr);

#endif  // POSITIONS_H
