#ifndef XBTOS_HAL_PORTS_H
#define XBTOS_HAL_PORTS_H

#include <types.h>

namespace hal {

inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

} // namespace hal

#endif
