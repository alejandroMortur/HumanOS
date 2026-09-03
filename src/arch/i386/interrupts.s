[BITS 32]

section .text

global idt_load
global irq0_stub
global irq1_stub
global context_switch
global task_switch
global scheduler_yield_asm
global syscall_stub

; Globals para ISRs (0 a 31)
global isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
global isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
global isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
global isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31

extern pit_handler
extern keyboard_handler
extern scheduler_yield_impl
extern schedule_needed
extern vga_puts
extern isr_handler
extern syscall_handler

idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

irq0_stub:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10        ; Segmento de datos del Kernel
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    cld                 ; Limpia la bandera de direccion requerida por C
    call pit_handler

    cmp byte [schedule_needed], 0
    je .no_schedule

    mov byte [schedule_needed], 0

    push esp            ; Pasar current_esp (apunta a GS) como argumento
    call scheduler_yield_impl
    add esp, 4

    test eax, eax
    jz .no_schedule

    mov esp, eax        ; Cambiar ESP al del nuevo proceso

.no_schedule:
    pop gs
    pop fs
    pop es
    pop ds
    popa
    iretd               ; Retorno de interrupcion

; Cambio de contexto simplificado usando jmp
; old_context: puntero a estructura cpu_context_t para guardar contexto actual
; new_context: puntero a estructura cpu_context_t para cargar nuevo contexto
context_switch:
    ; Guardar contexto actual
    mov eax, [esp + 4]      ; old_context
    mov [eax + 0], edi
    mov [eax + 4], esi
    mov [eax + 8], ebp
    mov [eax + 12], esp
    mov [eax + 16], ebx
    mov [eax + 20], edx
    mov [eax + 24], ecx
    mov [eax + 28], eax
    pushf
    pop dword [eax + 32]    ; eflags
    mov dword [eax + 36], 0 ; eip (no se usa)
    
    ; Cargar nuevo contexto y saltar a él
    mov eax, [esp + 8]      ; new_context
    mov edi, [eax + 0]
    mov esi, [eax + 4]
    mov ebp, [eax + 8]
    mov esp, [eax + 12]
    mov ebx, [eax + 16]
    mov edx, [eax + 20]
    mov ecx, [eax + 24]
    mov eax, [eax + 28]
    push dword [eax + 32]   ; eflags
    popf
    
    ; Saltar al nuevo eip
    jmp dword [eax + 36]

; Task switch usando iretd uniforme
; current_esp_ptr: puntero donde guardar el esp actual
; new_esp: nuevo stack pointer (con marco de interrupción falso)
task_switch:
    mov eax, [esp + 4]      ; current_esp_ptr
    mov ebx, [esp + 8]      ; new_esp
    mov [eax], esp          ; Guardar esp actual
    mov esp, ebx            ; Cambiar al nuevo esp
    iretd                   ; Restaurar contexto completo desde el stack

; Wrapper para scheduler_yield - guarda contexto como marco de interrupción
scheduler_yield_asm:
    pop eax                 ; EAX = Return EIP
    pushfd                  ; Apilar EFLAGS
    push dword 0x08         ; Apilar CS (Kernel Code Segment)
    push eax                ; Apilar Return EIP
    pusha                   ; Guardar registros generales
    push ds
    push es
    push fs
    push gs
    
    mov ax, 0x10            ; Segmento de datos del Kernel
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp                ; Pasar current_esp (apunta a GS) como argumento
    call scheduler_yield_impl  ; Llamar a implementación en C
    add esp, 4
    
    ; Si eax es 0, no cambiar ESP (no hay siguiente proceso)
    test eax, eax
    jz .no_switch
    
    ; Cambiar ESP al nuevo stack
    mov esp, eax
    
.no_switch:
    ; Restaurar los registros del siguiente proceso
    pop gs
    pop fs
    pop es
    pop ds
    popa
    iretd                   ; Retornar al siguiente proceso via iretd

irq1_stub:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10        ; Segmento de datos del Kernel
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    cld                 ; Limpia la bandera de direccion requerida por C
    call keyboard_handler

    pop gs
    pop fs
    pop es
    pop ds
    popa
    iretd               ; Retorno de interrupcion

; ===================================================================
; Stubs de Excepciones ISR (0 - 31)
; ===================================================================

isr_common_stub:
    pusha
    push ds

    mov ax, 0x10        ; Cargar segmento de datos del Kernel
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; Pasar puntero a registers_t (esp apunta a ds)
    cld
    call isr_handler
    add esp, 4          ; Limpiar argumento

    pop ds
    popa
    add esp, 8          ; Limpiar int_no y err_code
    iretd

%macro ISR_NOERRCODE 1
isr%1:
    push dword 0        ; Código de error ficticio
    push dword %1       ; Número de interrupción
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
isr%1:
    push dword %1       ; Número de interrupción (error code ya apilado por CPU)
    jmp isr_common_stub
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

; ===================================================================
; Stub de Llamadas al Sistema Syscall (int 0x80)
; ===================================================================

syscall_stub:
    push dword 0        ; Código de error ficticio
    push dword 0x80     ; Número de interrupción
    pusha
    push ds

    mov ax, 0x10        ; Cargar segmento de datos del Kernel
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; Pasar puntero a registers_t (esp apunta a ds)
    cld
    call syscall_handler
    add esp, 4          ; Limpiar argumento

    pop ds
    popa
    add esp, 8          ; Limpiar int_no y err_code
    iretd