#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <hal/console.h>

static void print_unsigned(unsigned int value, unsigned int base) {
    char buffer[32];
    unsigned int pos = 0;

    if (value == 0) {
        hal::print_char('0');
        return;
    }

    while (value > 0 && pos < sizeof(buffer)) {
        unsigned int digit = value % base;
        buffer[pos++] = (char)(digit < 10 ? ('0' + digit) : ('a' + digit - 10));
        value /= base;
    }

    while (pos > 0) {
        hal::print_char(buffer[--pos]);
    }
}

static void print_signed(int value) {
    if (value < 0) {
        hal::print_char('-');
        print_unsigned((unsigned int)(-value), 10);
    } else {
        print_unsigned((unsigned int)value, 10);
    }
}

extern "C" {

int putchar(int c) {
    hal::print_char((char)c);
    return c;
}

int puts(const char* text) {
    hal::print_string(text);
    hal::print_char('\n');
    return 0;
}

int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    for (unsigned int i = 0; format && format[i] != '\0'; ++i) {
        if (format[i] != '%') {
            hal::print_char(format[i]);
            continue;
        }

        ++i;
        if (format[i] == 's') {
            hal::print_string(va_arg(args, const char*));
        } else if (format[i] == 'd' || format[i] == 'i') {
            print_signed(va_arg(args, int));
        } else if (format[i] == 'u') {
            print_unsigned(va_arg(args, unsigned int), 10);
        } else if (format[i] == 'x') {
            print_unsigned(va_arg(args, unsigned int), 16);
        } else if (format[i] == 'c') {
            hal::print_char((char)va_arg(args, int));
        } else if (format[i] == '%') {
            hal::print_char('%');
        } else {
            hal::print_char('%');
            hal::print_char(format[i]);
        }
    }

    va_end(args);
    return 0;
}

int atoi(const char* text) {
    int sign = 1;
    int value = 0;

    while (isspace(*text)) {
        ++text;
    }
    if (*text == '-') {
        sign = -1;
        ++text;
    }
    while (isdigit(*text)) {
        value = value * 10 + (*text - '0');
        ++text;
    }
    return value * sign;
}

void abort() {
    hal::print_string("abort()\n");
    while (true) {
        __asm__ volatile("hlt");
    }
}

int isdigit(int c) {
    return c >= '0' && c <= '9';
}

int isalpha(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int isspace(int c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\v' || c == '\f';
}

int tolower(int c) {
    return (c >= 'A' && c <= 'Z') ? (c + 32) : c;
}

int toupper(int c) {
    return (c >= 'a' && c <= 'z') ? (c - 32) : c;
}

}
