#ifndef XBTOS_STDLIB_H
#define XBTOS_STDLIB_H

#include "stddef.h"

extern "C" {

void* malloc(size_t size);
void free(void* ptr);
int atoi(const char* text);
void abort();

}

#endif
