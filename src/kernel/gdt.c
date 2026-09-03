#include "gdt.h"

extern void gdt_flush(uint32_t);

// Definimos 6 segmentos: Nulo, Código Kernel, Datos Kernel, Código User, Datos User, TSS
static gdt_entry_t gdt_entries[6];
static gdt_ptr_t   gdt_ptr;
static tss_entry_t tss_entry;

extern void tss_flush(void);

static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

static void write_tss(int32_t num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t)&tss_entry;
    uint32_t limit = sizeof(tss_entry) - 1;

    gdt_set_gate(num, base, limit, 0xE9, 0x00); // 0xE9 = Present, Ring 3, 32-bit TSS

    uint8_t* ptr = (uint8_t*)&tss_entry;
    for (uint32_t i = 0; i < sizeof(tss_entry); i++) {
        ptr[i] = 0;
    }

    tss_entry.ss0 = ss0;
    tss_entry.esp0 = esp0;
    tss_entry.cs = KERNEL_CS | 3;
    tss_entry.ss = KERNEL_DS | 3;
    tss_entry.ds = KERNEL_DS | 3;
    tss_entry.es = KERNEL_DS | 3;
    tss_entry.fs = KERNEL_DS | 3;
    tss_entry.gs = KERNEL_DS | 3;
}

void tss_set_kernel_stack(uint32_t kstack) {
    tss_entry.esp0 = kstack;
}

void gdt_init(void) {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    // 0. Entrada Nula
    gdt_set_gate(0, 0, 0, 0, 0);

    // 1. Código Kernel (0x08)
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    // 2. Datos Kernel (0x10)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    // 3. Código Usuario (0x1B = 0x18 | 3)
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    // 4. Datos Usuario (0x23 = 0x20 | 3)
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    // 5. TSS (0x2B = 0x28 | 3)
    write_tss(5, KERNEL_DS, 0x0);

    // Cargar la GDT y el TSS en la CPU
    gdt_flush((uint32_t)&gdt_ptr);
    tss_flush();
}