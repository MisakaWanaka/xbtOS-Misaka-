#include "hal.h"
#include "console.h"
#include "ports.h"

namespace hal {

// =========================================================================
// 内部常量与数据结构隐藏
// =========================================================================
constexpr uint8_t IDT_DEFAULT_INT_GATE = 0x8E; // Present | Ring0 | 32-bit Int Gate

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

static IdtEntry idt[256];
static InterruptHandler interrupt_handlers[256] = { nullptr }; // 回调函数表

// PIC 常量
constexpr uint16_t PIC1_CMD = 0x20, PIC1_DATA = 0x21;
constexpr uint16_t PIC2_CMD = 0xA0, PIC2_DATA = 0xA1;
constexpr uint8_t  PIC_EOI  = 0x20;

// =========================================================================
// 外部汇编 ISR 声明 (通常定义在 interrupt.asm 中)
// =========================================================================
// 注意：你需要通过宏在汇编中定义 isr0 到 isr255，并在这里声明。
// 为了演示，这里假设有一个返回 ISR 地址数组的函数或手工声明：
extern "C" uint32_t isr_stub_table[]; 

// =========================================================================
// 内部辅助函数
// =========================================================================
static void idt_set_gate(uint8_t vector, uint32_t handler_addr, uint16_t selector, uint8_t flags) {
    idt[vector].offset_low  = (uint16_t)(handler_addr & 0xFFFF);
    idt[vector].selector    = selector;
    idt[vector].zero        = 0;
    idt[vector].type_attr   = flags;
    idt[vector].offset_high = (uint16_t)((handler_addr >> 16) & 0xFFFF);
}

static void pic_init() {
    outb(PIC1_CMD, 0x11);
    outb(PIC2_CMD, 0x11);
    
    outb(PIC1_DATA, 0x20); // 主片 IRQ 映射到 IDT 0x20 (32)
    outb(PIC2_DATA, 0x28); // 从片 IRQ 映射到 IDT 0x28 (40)
    
    outb(PIC1_DATA, 0x04); // 级联引脚
    outb(PIC2_DATA, 0x02);
    
    outb(PIC1_DATA, 0x01); // 8086 模式
    outb(PIC2_DATA, 0x01);
    
    // 默认屏蔽所有 IRQ
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

static void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);
}

// =========================================================================
// 核心：统一中断分发器 (由汇编的 common_stub 调用)
// =========================================================================
extern "C" void hal_interrupt_dispatcher(Registers* regs) {
    // 1. 查找是否注册了驱动/处理函数
    if (interrupt_handlers[regs->int_no] != nullptr) {
        InterruptHandler handler = interrupt_handlers[regs->int_no];
        handler(regs);
    } else {
        // 未处理的中断/异常处理逻辑
        // 如果是异常(0-31)，通常意味着内核 Panic
        // if (regs->int_no < 32) panic("Unhandled CPU Exception");
    }

    // 2. 如果这是硬件中断 (IRQ0-15，即 IDT 32-47)，必须发送 EOI 告知 PIC 中断结束
    if (regs->int_no >= 32 && regs->int_no <= 47) {
        pic_send_eoi(regs->int_no - 32);
    }
}

// =========================================================================
// HAL 公共接口实现
// =========================================================================

void init() {
    console_init();
    pic_init();

    // 加载 IDT，遍历装载所有的中断桩(stub)
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, isr_stub_table[i], 0x08, IDT_DEFAULT_INT_GATE);
    }

    IdtPointer idt_ptr = { sizeof(idt) - 1, (uint32_t)&idt[0] };
    __asm__ volatile("lidt %0" : : "m"(idt_ptr));

    // 开启必需的级联通道
    irq_enable(2);
}

void register_interrupt_handler(uint8_t interrupt_number, InterruptHandler handler) {
    interrupt_handlers[interrupt_number] = handler;
}

void unregister_interrupt_handler(uint8_t interrupt_number) {
    interrupt_handlers[interrupt_number] = nullptr;
}

void irq_enable(uint8_t irq) {
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    uint8_t bit = (irq < 8) ? irq : irq - 8;
    outb(port, inb(port) & ~(1 << bit));
}

void irq_disable(uint8_t irq) {
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    uint8_t bit = (irq < 8) ? irq : irq - 8;
    outb(port, inb(port) | (1 << bit));
}

void enable_interrupts()  { __asm__ volatile("sti"); }
void disable_interrupts() { __asm__ volatile("cli"); }
void halt_cpu()           { __asm__ volatile("hlt"); }

uint32_t get_cr2() {
    uint32_t val;
    __asm__ volatile("mov %%cr2, %0" : "=r"(val));
    return val;
}

} // namespace hal