; Declaración del estándar Multiboot
MBALIGN  equ  1 << 0
MEMINFO  equ  1 << 1
FLAGS    equ  MBALIGN | MEMINFO
MAGIC    equ  0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom:
    resb 16384 ; Reservamos 16 KB para el Stack
stack_top:

section .text
global _start:function (_start.end - _start)
extern kernel_main

_start:
    mov esp, stack_top
    push ebx                ; Pasar multiboot info structure
    push eax                ; Pasar multiboot magic
    call kernel_main
    cli
.hang:
    hlt
    jmp .hang
.end: