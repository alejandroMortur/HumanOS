#include "cpu.h"
#include "vga.h"
#include <stdint.h>

static inline void cpuid(uint32_t code, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
    __asm__ __volatile__(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(code)
    );
}

// Obtener datos del procesador vía instrucción CPUID
void cpu_get_info(cpu_info_t* info) {
    if (info == ((void*)0)) return;

    uint32_t eax, ebx, ecx, edx;

    // 1. Obtener Vendor String (CPUID leaf 0)
    cpuid(0, &eax, &ebx, &ecx, &edx);
    ((uint32_t*)info->vendor)[0] = ebx;
    ((uint32_t*)info->vendor)[1] = edx;
    ((uint32_t*)info->vendor)[2] = ecx;
    info->vendor[12] = '\0';

    // 2. Obtener Cores & Modelo (CPUID leaf 1)
    cpuid(1, &eax, &ebx, &ecx, &edx);
    info->cores = (ebx >> 16) & 0xFF;
    if (info->cores == 0) info->cores = 1;
    info->family = (eax >> 8) & 0x0F;
    info->model = (eax >> 4) & 0x0F;

    // 3. Obtener Brand String completo (CPUID leaf 0x80000002..0x80000004)
    cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    if (eax >= 0x80000004) {
        uint32_t* brand_ptr = (uint32_t*)info->brand;
        cpuid(0x80000002, &brand_ptr[0], &brand_ptr[1], &brand_ptr[2], &brand_ptr[3]);
        cpuid(0x80000003, &brand_ptr[4], &brand_ptr[5], &brand_ptr[6], &brand_ptr[7]);
        cpuid(0x80000004, &brand_ptr[8], &brand_ptr[9], &brand_ptr[10], &brand_ptr[11]);
        info->brand[48] = '\0';
    } else {
        int i = 0;
        const char* default_brand = "x86 Processor";
        while (default_brand[i] != '\0' && i < 48) {
            info->brand[i] = default_brand[i];
            i++;
        }
        info->brand[i] = '\0';
    }
}

// Imprimir información de hardware procesador
void cpu_print_info(void) {
    cpu_info_t cpu;
    cpu_get_info(&cpu);

    vga_puts("  CPU Vendor : ", COLOR_LIGHT_GREEN);
    vga_puts(cpu.vendor, COLOR_WHITE);
    vga_putc('\n', COLOR_WHITE);

    vga_puts("  CPU Model  : ", COLOR_LIGHT_GREEN);
    vga_puts(cpu.brand, COLOR_WHITE);
    vga_putc('\n', COLOR_WHITE);

    vga_puts("  CPU Cores  : ", COLOR_LIGHT_GREEN);
    char buf[16];
    int count = cpu.cores;
    int j = 0;
    if (count == 0) buf[j++] = '0';
    while (count > 0 && j < 15) {
        buf[j++] = '0' + (count % 10);
        count /= 10;
    }
    for (int k = j - 1; k >= 0; k--) vga_putc(buf[k], COLOR_WHITE);
    vga_puts(" Logical Core(s)\n", COLOR_WHITE);
}
