#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// Estructura de una entrada de la GDT (8 bytes)
struct gdt_entry_struct {
    uint16_t limit_low;    // Los 16 bits inferiores del límite
    uint16_t base_low;     // Los 16 bits inferiores de la dirección base
    uint8_t  base_middle;  // Los siguientes 8 bits de la dirección base
    uint8_t  access;       // Bits de acceso (privilegio, tipo de segmento)
    uint8_t  granularity;  // Tamaño del límite y flags (32-bit, granularidad de 4K)
    uint8_t  base_high;    // Los últimos 8 bits de la dirección base
} __attribute__((packed));

typedef struct gdt_entry_struct gdt_entry_t;

// Puntero especial que la instrucción 'lgdt' de la CPU espera recibir
struct gdt_ptr_struct {
    uint16_t limit;        // Tamaño total de la GDT menos 1
    uint32_t base;         // Dirección de memoria donde empieza la GDT
} __attribute__((packed));

typedef struct gdt_ptr_struct gdt_ptr_t;

// Estructura del Task State Segment (TSS) de x86
struct tss_entry_struct {
    uint32_t prev_tss;
    uint32_t esp0;       // Stack pointer de kernel al ocurrir interrupciones en Ring 3
    uint32_t ss0;        // Segmento de pila de kernel (0x10)
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

typedef struct tss_entry_struct tss_entry_t;

// Selectores de Segmento de la GDT
#define KERNEL_CS 0x08
#define KERNEL_DS 0x10
#define USER_CS   0x1B // Descriptor 3 con RPL=3 (0x18 | 3)
#define USER_DS   0x23 // Descriptor 4 con RPL=3 (0x20 | 3)
#define TSS_SEG   0x2B // Descriptor 5 con RPL=3 (0x28 | 3)

void gdt_init(void);
void tss_set_kernel_stack(uint32_t kstack);

#endif