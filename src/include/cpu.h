#ifndef CPU_H
#define CPU_H

#include <stdint.h>

typedef struct {
    char vendor[13];       // p.ej. "GenuineIntel", "AuthenticAMD"
    char brand[49];        // p.ej. "Intel(R) Core(TM) i7-10700K CPU @ 3.80GHz"
    uint32_t cores;        // Número de núcleos lógicos
    uint32_t family;
    uint32_t model;
} cpu_info_t;

// Funciones de detección de procesador (CPUID)
void cpu_get_info(cpu_info_t* info);
void cpu_print_info(void);

#endif
