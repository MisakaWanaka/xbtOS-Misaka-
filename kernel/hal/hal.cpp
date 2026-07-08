#include "hal.h"
#include "console.h"
#include "ports.h"

namespace hal {

static IdtEntry idt[256];

static void pic_initialize() {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0xFD);
    outb(0xA1, 0xFF);
}

static void pic_end_of_interrupt(uint8_t irq) {
    if (irq >= 8) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
}

static const InterruptController pic = {
    pic_initialize,
    pic_end_of_interrupt
};

static void load_idt() {
    IdtPointer idt_ptr;
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base = (uint32_t)&idt[0];
    __asm__ volatile("lidt %0" : : "m"(idt_ptr));
}

void hal_init() {
    console_init();
    pic.initialize();
    load_idt();
}

void enable_interrupts() {
    __asm__ volatile("sti");
}

void halt_cpu() {
    __asm__ volatile("hlt");
}

uint8_t keyboard_read_scancode() {
    return inb(0x60);
}

const InterruptController& interrupt_controller() {
    return pic;
}

void idt_set_gate(uint8_t vector, void (*handler)(), uint16_t selector, uint8_t flags) {
    uint32_t address = (uint32_t)handler;
    idt[vector].offset_low = (uint16_t)(address & 0xFFFF);
    idt[vector].selector = selector;
    idt[vector].zero = 0;
    idt[vector].type_attr = flags;
    idt[vector].offset_high = (uint16_t)((address >> 16) & 0xFFFF);
}

} // namespace hal
