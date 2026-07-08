#include <types.h>
#include "heap.h"

extern "C" {

void* malloc(size_t size) {
    return kmalloc((uint32_t)size);
}

void free(void* ptr) {
    if (ptr != nullptr) {
        kfree(ptr);
    }
}

void __cxa_pure_virtual() {
    while (true) {
        __asm__ volatile("hlt");
    }
}

}
