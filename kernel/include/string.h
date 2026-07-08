#ifndef XBTOS_STRING_H
#define XBTOS_STRING_H

#include "types.h"

inline void* kmemset(void* ptr, int value, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    while (num--) {
        *p++ = (unsigned char)value;
    }
    return ptr;
}

inline void* kmemcpy(void* destination, const void* source, size_t num) {
    unsigned char* dest = (unsigned char*)destination;
    const unsigned char* src = (const unsigned char*)source;
    while (num--) {
        *dest++ = *src++;
    }
    return destination;
}

inline int kstrcmp(const char* str1, const char* str2) {
    while (*str1 && (*str1 == *str2)) {
        ++str1;
        ++str2;
    }
    return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}

inline size_t kstrlen(const char* str) {
    if (!str) {
        return 0;
    }
    size_t len = 0;
    while (str[len] != '\0') {
        ++len;
    }
    return len;
}

inline char* kstrcpy(char* dest, const char* src) {
    char* start = dest;
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return start;
}

inline char* kstrncpy(char* dest, const char* src, size_t max_len) {
    size_t i = 0;
    for (; i + 1 < max_len && src[i] != '\0'; ++i) {
        dest[i] = src[i];
    }
    if (max_len > 0) {
        dest[i] = '\0';
    }
    return dest;
}

#endif
