cmake_minimum_required(VERSION 3.20)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_C_COMPILER "E:/x86_64-elf-tools-windows/bin/x86_64-elf-gcc.exe")
set(CMAKE_CXX_COMPILER "E:/x86_64-elf-tools-windows/bin/x86_64-elf-g++.exe")
set(CMAKE_ASM_NASM_COMPILER "C:/Program Files/NASM/nasm.exe")

project(XBTOS C CXX ASM_NASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_language(ASM_NASM)
set(CMAKE_ASM_NASM_COMPILE_OBJECT
        "<CMAKE_ASM_NASM_COMPILER> -f elf32 -o <OBJECT> <SOURCE>"
)

add_executable(XBTOS.elf
        boot/boot.asm
        kernel/main.cpp
        kernel/app/app.cpp
        kernel/app/app.h
        kernel/app/third_party.h
        kernel/hal/hal.cpp
        kernel/hal/hal.h
        kernel/hal/ports.h
        kernel/hal/console.cpp
        kernel/hal/console.h
        kernel/i18n/i18n.cpp
        kernel/i18n/i18n.h
        kernel/fs/vfs.cpp
        kernel/fs/vfs.h
        kernel/include/types.h
        kernel/include/string.h
        kernel/include/stdarg.h
        kernel/include/stddef.h
        kernel/include/stdint.h
        kernel/include/stdlib.h
        kernel/include/stdio.h
        kernel/include/ctype.h
        kernel/memory/physmem.cpp
        kernel/memory/physmem.h
        kernel/memory/heap.cpp
        kernel/memory/heap.h
        kernel/memory/stdlib.cpp
        kernel/memory/libc.cpp
)

target_include_directories(XBTOS.elf PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel
        ${CMAKE_CURRENT_SOURCE_DIR}/kernel/include
)

target_compile_options(XBTOS.elf PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-m32>
        $<$<COMPILE_LANGUAGE:CXX>:-ffreestanding>
        $<$<COMPILE_LANGUAGE:CXX>:-nostdlib>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-pie>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-stack-protector>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
        -g -O0 -Wall
)

set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/linker.ld")

target_link_options(XBTOS.elf PRIVATE
        -m32
        -nostdlib
        -static
        -Wl,-m,elf_i386
        LINKER:-T,${LINKER_SCRIPT}
)
