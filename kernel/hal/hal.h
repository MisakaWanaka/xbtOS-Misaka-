#ifndef XBTOS_HAL_HAL_H
#define XBTOS_HAL_HAL_H

#include <types.h>

namespace hal {

struct IdtEntry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct IdtPointer {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct InterruptController {
    void (*initialize)();
    void (*end_of_interrupt)(uint8_t irq);
};

void hal_init();
void enable_interrupts();
void halt_cpu();
uint8_t keyboard_read_scancode();
const InterruptController& interrupt_controller();
void idt_set_gate(uint8_t vector, void (*handler)(), uint16_t selector, uint8_t flags);

} // namespace hal

#endif
