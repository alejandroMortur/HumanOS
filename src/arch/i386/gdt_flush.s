global gdt_flush
global tss_flush
extern gdt_ptr

gdt_flush:
    mov eax, [esp + 4]  ; Toma el puntero a la GDT pasado como argumento
    lgdt [eax]          ; Carga la GDT en el registro GDTR del procesador

    ; Recargamos los registros de datos con el segmento de datos del kernel (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Salto lejano (far jump) para recargar el registro CS con el segmento de código (0x08)
    jmp 0x08:.flush

.flush:
    ret

tss_flush:
    mov ax, 0x2B        ; Cargar selector TSS (0x28 | 3)
    ltr ax              ; Cargar Task Register
    ret