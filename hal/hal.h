#ifndef XBTOS_HAL_HAL_H
#define XBTOS_HAL_HAL_H

#include <types.h>

namespace hal {

// =========================================================================
// CPU 中断上下文 (由底层的汇编 ISR Stub 压栈)
// =========================================================================
struct Registers {
    uint32_t ds;                                     // 数据段选择子
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // PUSHA 压入的通用寄存器
    uint32_t int_no, err_code;                       // 中断号 和 错误代码 (部分异常由CPU自动压入，其它手动压入0)
    uint32_t eip, cs, eflags, useresp, ss;           // CPU自动压入的状态 (如果在特权级间跳转还有 useresp 和 ss)
} __attribute__((packed));

// 中断处理函数指针类型
typedef void (*InterruptHandler)(Registers* regs);

// =========================================================================
// CPU 标准异常号定义 (0-31)
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
    // 15 is reserved
    X87_FLOATING_POINT_EXCEPTION = 16,
    ALIGNMENT_CHECK = 17,
    MACHINE_CHECK = 18,
    SIMD_FLOATING_POINT_EXCEPTION = 19,
    VIRTUALIZATION_EXCEPTION = 20,
    CONTROL_PROTECTION_EXCEPTION = 21
};

// =========================================================================
// HAL 导出函数
// =========================================================================

// 1. 系统初始化
void init();

// 2. 中断处理与路由
// 供外部驱动使用：注册特定的中断/异常处理函数
void register_interrupt_handler(uint8_t interrupt_number, InterruptHandler handler);
void unregister_interrupt_handler(uint8_t interrupt_number);

// 3. 硬件 IRQ 控制 (封装了 8259A PIC 的细节)
void irq_enable(uint8_t irq);   // 开启指定的硬件中断 (例如 IRQ1 键盘)
void irq_disable(uint8_t irq);  // 屏蔽指定的硬件中断

// 4. CPU 指令级控制
void enable_interrupts();       // sti
void disable_interrupts();      // cli
void halt_cpu();                // hlt
uint32_t get_cr2();             // 获取触发 Page Fault 的内存地址

} // namespace hal

#endif // XBTOS_HAL_HAL_H