#ifndef XBTOS_HAL_CONSOLE_H
#define XBTOS_HAL_CONSOLE_H

#include <types.h>

namespace hal {

struct ConsoleDevice {
    void (*clear)();
    void (*put_char)(char c);
    void (*write)(const char* text);
    void (*write_utf8)(const char* text);
};

void console_init();
const ConsoleDevice& console();
void clear_screen();
void print_char(char c);
void print_string(const char* text);
void print_utf8(const char* text);

} // namespace hal

extern "C" void clear_screen();
extern "C" void print_char(char c);
extern "C" void print_string(const char* text);

#endif
