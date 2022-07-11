#ifndef COPY_H
#define COPY_H

#include "scr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int cp(const char *from, const char *to);

#endif // COPY_H
