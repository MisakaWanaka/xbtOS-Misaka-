#ifndef XBTOS_HAL_HAL_H
#define XBTOS_HAL_HAL_H

#include <types.h>
#include <stddef.h>

namespace hal {

// =========================================================================
// CPU 中断上下文（匹配 interrupt.S 压栈顺序）
// =========================================================================
struct Registers {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} __attribute__((packed));

// 中断处理函数指针
typedef void (*InterruptHandler)(Registers* regs);

// =========================================================================
// CPU 异常枚举（0-31）
// =========================================================================
enum CpuException : uint8_t {
    DIVIDE_BY_ZERO = 0,
    DEBUG = 1,
    NON_MASKABLE_INTERRUPT = 2,
    BREAKPOINT = 3,
    OVERFLOW = 4,
    BOUND_RANGE_EXCEEDED = 5,
    INVALID_OPCODE = 6,
    DEVICE_NOT_AVAILABLE = 7,
    DOUBLE_FAULT = 8,
    COPROCESSOR_SEGMENT_OVERRUN = 9,
    INVALID_TSS = 10,
    SEGMENT_NOT_PRESENT = 11,
    STACK_SEGMENT_FAULT = 12,
    GENERAL_PROTECTION_FAULT = 13,
    PAGE_FAULT = 14,
    X87_FLOATING_POINT_EXCEPTION = 16,
    ALIGNMENT_CHECK = 17,
    MACHINE_CHECK = 18,
    SIMD_FLOATING_POINT_EXCEPTION = 19,
    VIRTUALIZATION_EXCEPTION = 20,
    CONTROL_PROTECTION_EXCEPTION = 21
};

// =========================================================================
// HAL 公共接口
// =========================================================================

// 1. 系统初始化（自动初始化时钟、键盘、APIC 等）
void init();

// 2. 中断处理
void register_interrupt_handler(uint8_t interrupt_number, InterruptHandler handler);
void unregister_interrupt_handler(uint8_t interrupt_number);

// 3. IRQ 控制（支持 PIC 和 APIC 两种模式）
void irq_enable(uint8_t irq);
void irq_disable(uint8_t irq);

// 4. CPU 指令
void enable_interrupts();
void disable_interrupts();
void halt_cpu();
uint32_t get_cr2();

// 5. 系统时钟（基于 PIT，精度约 1ms）
void init_timer(uint32_t frequency);    // 设置时钟频率（Hz），通常 1000
uint64_t get_ticks();                   // 获取自启动以来的滴答数
void msleep(uint32_t ms);              // 忙等待（轮询）延迟，适合短时等待

// 6. 键盘（PS/2 扫描码）
void keyboard_init();                  // 启用键盘中断，注册处理函数
uint8_t keyboard_read_scancode();      // 非阻塞读取，若无按键返回 0

// 7. 上下文切换（用于任务调度）
void switch_context(Registers** old_regs, Registers* new_regs);
    // 保存当前寄存器到 old_regs，加载 new_regs 并切换

// 8. APIC 支持（可选，自动检测并启用）
bool is_apic_available();
void apic_init();                      // 初始化本地 APIC 和 I/O APIC

// 9. 调试辅助
void dump_registers(Registers* regs);  // 打印所有寄存器

} // namespace hal

#endif // XBTOS_HAL_HAL_H