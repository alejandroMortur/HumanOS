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

void gdt_init(void);

#endif