#ifndef IDT_H
#define IDT_H

#include <stdint.h>

// Estructura de una entrada de la IDT (8 bytes)
struct idt_entry_struct {
    uint16_t base_low;   // 16 bits inferiores de la dirección del handler
    uint16_t sel;        // Selector de segmento de código (Kernel = 0x08)
    uint8_t  always0;    // Siempre 0 (requerido por x86)
    uint8_t  flags;      // Tipo de puerta, nivel de privilegio (Ring 0)
    uint16_t base_high;  // 16 bits superiores de la dirección del handler
} __attribute__((packed));

typedef struct idt_entry_struct idt_entry_t;

// Puntero para la instrucción 'lidt'
struct idt_ptr_struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

typedef struct idt_ptr_struct idt_ptr_t;

void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

#endif