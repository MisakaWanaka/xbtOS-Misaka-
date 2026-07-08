/*
 * interrupt.S - 生成 256 个中断桩，并导出 isr_stub_table
 * 使用 GNU as (AT&T 语法)，与 hal.cpp 协同工作
 */

.text
.code32

/* 宏：生成无错误码的中断桩（压入假错误码 0） */
.macro ISR_NOERRCODE vector
    .global isr\vector
    isr\vector:
        pushl $0                 /* 压入假错误码 */
        pushl $\vector           /* 压入中断号 */
        jmp common_stub
.endm

/* 宏：生成有错误码的中断桩（CPU 已压入错误码） */
.macro ISR_ERRCODE vector
    .global isr\vector
    isr\vector:
        pushl $\vector           /* 压入中断号 */
        jmp common_stub
.endm

/*
 * 定义哪些向量自带错误码（按 x86 规范）
 * 向量：8, 10, 11, 12, 13, 14, 17, 21, 29, 30
 * 使用位图标记，便于 .if 判断
 */
.set ERRCODE_LIST, 0
.set ERRCODE_LIST, ERRCODE_LIST | (1 << 8)
.set ERRCODE_LIST, ERRCODE_LIST | (1 << 10)
.set ERRCODE_LIST, ERRCODE_LIST | (1 << 11)
.set ERRCODE_LIST, ERRCODE_LIST | (1 << 12)
.set ERRCODE_LIST, ERRCODE_LIST | (1 << 13)
.set ERRCODE_LIST, ERRCODE_LIST | (1 << 14)
.set ERRCODE_LIST, ERRCODE_LIST | (1 << 17)
.set ERRCODE_LIST, ERRCODE_LIST | (1 << 21)
.set ERRCODE_LIST, ERRCODE_LIST | (1 << 29)
.set ERRCODE_LIST, ERRCODE_LIST | (1 << 30)

/* 宏：根据向量选择使用哪种桩 */
.macro isr_gen vector
    .if (ERRCODE_LIST >> \vector) & 1
        ISR_ERRCODE \vector
    .else
        ISR_NOERRCODE \vector
    .endif
.endm

/* 生成 isr0 ~ isr255 */
.set i, 0
.rept 256
    isr_gen i
    .set i, i+1
.endr

/* --------------------------------------------------------------------
 * 公共中断分发桩：保存所有寄存器，调用 hal_interrupt_dispatcher
 * 之后恢复并返回
 * -------------------------------------------------------------------- */
.extern hal_interrupt_dispatcher

common_stub:
    /* 此时堆栈：[中断号, 错误码, eip, cs, eflags, ...] (CPU压入) */
    /* 保存所有段寄存器和通用寄存器 */
    pushal
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    /* 加载内核数据段 (假设选择子 0x10) */
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    /* 此时堆栈布局与 hal::Registers 完全一致：
     *   gs, fs, es, ds, edi, esi, ebp, esp, ebx, edx, ecx, eax,
     *   int_no, err_code, eip, cs, eflags, useresp, ss
     * 注意 pushal 压入顺序为 eax, ecx, edx, ebx, esp, ebp, esi, edi，
     * 但堆栈上从低到高是 edi, esi, ebp, esp, ebx, edx, ecx, eax，
     * 我们按此排列 Registers。
     */
    movl %esp, %eax          /* 将栈顶指针作为参数 (Registers*) */
    pushl %eax
    call hal_interrupt_dispatcher
    addl $4, %esp            /* 清理参数 */

    /* 恢复寄存器 */
    popl %gs
    popl %fs
    popl %es
    popl %ds
    popal

    /* 清理堆栈上由桩压入的中断号和错误码（两个 dword） */
    addl $8, %esp
    iret

/* --------------------------------------------------------------------
 * 导出 isr_stub_table 数组
 * -------------------------------------------------------------------- */
.section .data
.global isr_stub_table
isr_stub_table:
    .set i, 0
    .rept 256
        .long isr\i
        .set i, i+1
    .endr