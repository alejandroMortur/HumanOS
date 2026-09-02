[BITS 32]

global idt_load
global irq0_stub
global irq1_stub
global context_switch
global task_switch
global scheduler_yield_asm
extern pit_handler
extern keyboard_handler
extern scheduler_yield_impl
extern schedule_needed

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

; Wrapper para scheduler_yield - guarda contexto como interrupción
scheduler_yield_asm:
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
    
    call scheduler_yield_impl  ; Llamar a implementación en C
    
    pop gs
    pop fs
    pop es
    pop ds
    popa
    iretd                   ; Retornar al siguiente proceso

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