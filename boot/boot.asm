[BITS 32]

MBOOT_MAGIC     equ 0x1BADB002
MBOOT_MEM_INFO  equ 1 << 0
MBOOT_FLAGS     equ MBOOT_MEM_INFO
MBOOT_CHECKSUM  equ -(MBOOT_MAGIC + MBOOT_FLAGS)

section .multiboot
align 4
    dd MBOOT_MAGIC
    dd MBOOT_FLAGS
    dd MBOOT_CHECKSUM

section .data
align 8
gdt_start:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ 0x08
DATA_SEG equ 0x10

extern kernel_main
extern keyboard_handler

global _start
global keyboard_handler_asm

section .text
_start:
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    jmp CODE_SEG:.flush_cs

.flush_cs:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    call kernel_main

.hang:
    hlt
    jmp .hang

keyboard_handler_asm:
    pusha
    call keyboard_handler
    popa
    iret
