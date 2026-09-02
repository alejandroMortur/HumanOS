#include "gdt.h"

extern void gdt_flush(uint32_t);

// Definimos 3 segmentos: Nulo, Código Kernel y Datos Kernel
static gdt_entry_t gdt_entries[3];
static gdt_ptr_t   gdt_ptr;

static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

void gdt_init(void) {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 3) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    // 1. Entrada Nula (Requisito obligatorio de la arquitectura x86)
    gdt_set_gate(0, 0, 0, 0, 0);

    // 2. Segmento de Código del Kernel (Base: 0, Límite: 4GB, Ring 0, Ejecutable)
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    // 3. Segmento de Datos del Kernel (Base: 0, Límite: 4GB, Ring 0, Lectura/Escritura)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    // Cargar la GDT en la CPU
    gdt_flush((uint32_t)&gdt_ptr);
}