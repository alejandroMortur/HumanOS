#ifndef ISR_H
#define ISR_H

#include <stdint.h>

// Estructura que representa los registros apilados durante una excepción/ISR
typedef struct {
    uint32_t ds;                                           // Segmento de datos
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax; // Apilados por PUSHA
    uint32_t int_no, err_code;                             // Número de interrupción y código de error
    uint32_t eip, cs, eflags;                              // Apilados por la CPU automáticamente
} registers_t;

// Función de manejo de ISR en C
void isr_handler(registers_t* regs);

#endif
