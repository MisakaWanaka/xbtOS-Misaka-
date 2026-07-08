#ifndef XBTOS_STDIO_H
#define XBTOS_STDIO_H

extern "C" {

int putchar(int c);
int puts(const char* text);
int printf(const char* format, ...);

}

#endif
