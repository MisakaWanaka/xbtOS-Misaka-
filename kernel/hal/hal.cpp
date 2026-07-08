#include "hal.h"
#include "console.h"
#include "ports.h"
#include <stddef.h>
#include <string.h>

namespace hal {

// =========================================================================
// 内部常量与数据结构
// =========================================================================
constexpr uint8_t IDT_DEFAULT_INT_GATE = 0x8E;

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
static InterruptHandler interrupt_handlers[256] = { nullptr };

// PIC 常量
constexpr uint16_t PIC1_CMD = 0x20, PIC1_DATA = 0x21;
constexpr uint16_t PIC2_CMD = 0xA0, PIC2_DATA = 0xA1;
constexpr uint8_t  PIC_EOI  = 0x20;

// APIC 寄存器地址（需根据硬件映射）
#define APIC_BASE_ADDR 0xFEE00000
#define APIC_EOI       0x0B0
#define APIC_LVT_TIMER 0x320
#define APIC_TIMER_ICR 0x380
#define APIC_TIMER_CCR 0x390
#define APIC_TIMER_DCR 0x3E0
#define APIC_SPURIOUS  0x0F0

// =========================================================================
// 外部汇编符号
// =========================================================================
extern "C" uint32_t isr_stub_table[];

// =========================================================================
// 内部变量
// =========================================================================
static bool apic_enabled = false;
static volatile uint64_t system_ticks = 0;
static volatile uint8_t last_scancode = 0;
static bool keyboard_ready = false;

// =========================================================================
// 内部辅助函数
// =========================================================================

// ---- IDT 设置 ----
static void idt_set_gate(uint8_t vector, uint32_t handler_addr, uint16_t selector, uint8_t flags) {
    idt[vector].offset_low  = (uint16_t)(handler_addr & 0xFFFF);
    idt[vector].selector    = selector;
    idt[vector].zero        = 0;
    idt[vector].type_attr   = flags;
    idt[vector].offset_high = (uint16_t)((handler_addr >> 16) & 0xFFFF);
}

// ---- PIC 操作 ----
static void pic_init() {
    outb(PIC1_CMD, 0x11);
    outb(PIC2_CMD, 0x11);
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

static void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);
}

// ---- APIC 操作 ----
static inline void apic_write(uint32_t reg, uint32_t value) {
    *((volatile uint32_t*)(APIC_BASE_ADDR + reg)) = value;
}
static inline uint32_t apic_read(uint32_t reg) {
    return *((volatile uint32_t*)(APIC_BASE_ADDR + reg));
}

static void apic_init_internal() {
    // 检测是否支持 APIC（通过 CPUID）
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    if (!(edx & (1 << 9))) {
        // 不支持 APIC
        apic_enabled = false;
        return;
    }

    // 启用本地 APIC（设置 IA32_APIC_BASE MSR）
    uint32_t msr_low, msr_high;
    __asm__ volatile("rdmsr" : "=a"(msr_low), "=d"(msr_high) : "c"(0x1B));
    msr_low |= (1 << 11);  // 设置 APIC Enable 位
    __asm__ volatile("wrmsr" : : "a"(msr_low), "d"(msr_high), "c"(0x1B));

    // 配置 Spurious Interrupt Vector Register 启用 APIC
    apic_write(APIC_SPURIOUS, apic_read(APIC_SPURIOUS) | 0x100 | 0xFF);

    // 设置 LVT Timer 为周期性模式，向量设为 0x20（与 PIC 一致）
    apic_write(APIC_LVT_TIMER, 0x20 | 0x20000); // 周期性

    // 设置除计时器外的所有 LVT 条目为屏蔽状态（简化）
    // 实际应配置 IOAPIC 重定向，这里略

    apic_enabled = true;
}

// ---- 异常默认处理器 ----
static void default_exception_handler(Registers* regs) {
    console_write("*** CPU EXCEPTION ***\n");
    console_write("Vector: ");
    console_write_dec(regs->int_no);
    console_write("  Error code: 0x");
    console_write_hex(regs->err_code);
    console_write("\nEIP: 0x");
    console_write_hex(regs->eip);
    console_write("  CS: 0x");
    console_write_hex(regs->cs);
    console_write("  EFLAGS: 0x");
    console_write_hex(regs->eflags);
    console_write("\n");
    dump_registers(regs);
    // 停机
    disable_interrupts();
    while (1) halt_cpu();
}

// ---- PIT 时钟中断处理 ----
static void pit_handler(Registers*) {
    system_ticks++;
}

// ---- 键盘中断处理 ----
static void keyboard_handler(Registers*) {
    uint8_t status = inb(0x64);
    if (status & 0x01) {
        uint8_t scancode = inb(0x60);
        last_scancode = scancode;
        keyboard_ready = true;
    }
}

// =========================================================================
// 核心中断分发器（由汇编 common_stub 调用）
// =========================================================================
extern "C" void hal_interrupt_dispatcher(Registers* regs) {
    // 调用已注册的处理函数
    if (interrupt_handlers[regs->int_no] != nullptr) {
        interrupt_handlers[regs->int_no](regs);
    } else {
        // 默认未处理：如果是异常则触发默认处理器
        if (regs->int_no < 32) {
            default_exception_handler(regs);
        }
    }

    // 发送 EOI
    if (apic_enabled) {
        // 使用 APIC EOI
        apic_write(APIC_EOI, 0);
    } else {
        if (regs->int_no >= 32 && regs->int_no <= 47) {
            pic_send_eoi(regs->int_no - 32);
        }
    }
}

// =========================================================================
// 公共接口实现
// =========================================================================

void init() {
    console_init();
    pic_init();

    // 尝试初始化 APIC
    apic_init_internal();

    // 加载 IDT
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, isr_stub_table[i], 0x08, IDT_DEFAULT_INT_GATE);
    }
    IdtPointer idt_ptr = { sizeof(idt) - 1, (uint32_t)&idt[0] };
    __asm__ volatile("lidt %0" : : "m"(idt_ptr));

    // 为异常注册默认处理器
    for (int i = 0; i < 32; i++) {
        register_interrupt_handler(i, default_exception_handler);
    }

    // 开启级联 PIC 通道
    if (!apic_enabled) {
        irq_enable(2);
    }

    // 初始化时钟（默认 1000Hz）
    init_timer(1000);

    // 初始化键盘
    keyboard_init();

    // 开启中断
    enable_interrupts();
}

void register_interrupt_handler(uint8_t interrupt_number, InterruptHandler handler) {
    interrupt_handlers[interrupt_number] = handler;
}

void unregister_interrupt_handler(uint8_t interrupt_number) {
    interrupt_handlers[interrupt_number] = nullptr;
}

void irq_enable(uint8_t irq) {
    if (apic_enabled) {
        // APIC 模式下，通过 IOAPIC 控制，这里简化：不处理
        // 实际需操作 IOAPIC 重定向表
        return;
    }
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    uint8_t bit = (irq < 8) ? irq : irq - 8;
    outb(port, inb(port) & ~(1 << bit));
}

void irq_disable(uint8_t irq) {
    if (apic_enabled) {
        return;
    }
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

// ---- 时钟 ----
void init_timer(uint32_t frequency) {
    if (frequency == 0) return;
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
    register_interrupt_handler(32, pit_handler);
    irq_enable(0);
}

uint64_t get_ticks() {
    return system_ticks;
}

void msleep(uint32_t ms) {
    uint64_t start = system_ticks;
    while ((system_ticks - start) < ms) {
        halt_cpu();  // 空闲等待，中断会唤醒
    }
}

// ---- 键盘 ----
void keyboard_init() {
    // 启用键盘中断（IRQ1）
    register_interrupt_handler(33, keyboard_handler);
    irq_enable(1);
}

uint8_t keyboard_read_scancode() {
    if (!keyboard_ready) return 0;
    uint8_t code = last_scancode;
    keyboard_ready = false;
    return code;
}

// ---- 上下文切换 ----
void switch_context(Registers** old_regs, Registers* new_regs) {
    // 保存当前 CPU 上下文到 old_regs
    // 由于我们的中断分发器已经保存了完整上下文，此处利用它
    // 实际实现中，可以调用类似 __asm__ volatile("mov %%esp, %0" : "=r"(old_regs));
    // 但为了与中断上下文兼容，我们采用模拟：将当前栈顶指针保存为 Registers*
    Registers* current;
    __asm__ volatile("mov %%esp, %0" : "=r"(current));
    // 注意：current 指向的是当前栈，但可能不是完整的 Registers 结构，需谨慎。
    // 更稳健的做法是使用专用函数保存，这里作为示例。
    *old_regs = current;
    // 切换到新栈
    __asm__ volatile("mov %0, %%esp" : : "r"(new_regs));
    // 返回后，新任务将从 new_regs->eip 继续执行（需在任务创建时设置好返回地址）
}

// ---- APIC 状态查询 ----
bool is_apic_available() {
    return apic_enabled;
}

void apic_init() {
    apic_init_internal();
}

// ---- 调试 ----
void dump_registers(Registers* regs) {
    console_write("EAX: ");
    console_write_hex(regs->eax);
    console_write("  EBX: ");
    console_write_hex(regs->ebx);
    console_write("  ECX: ");
    console_write_hex(regs->ecx);
    console_write("  EDX: ");
    console_write_hex(regs->edx);
    console_write("\n");
    console_write("ESI: ");
    console_write_hex(regs->esi);
    console_write("  EDI: ");
    console_write_hex(regs->edi);
    console_write("  EBP: ");
    console_write_hex(regs->ebp);
    console_write("  ESP: ");
    console_write_hex(regs->esp);
    console_write("\n");
}

} // namespace hal