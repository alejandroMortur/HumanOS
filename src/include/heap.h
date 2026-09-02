#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>

// Tamaño mínimo de bloque para heap
#define HEAP_MIN_BLOCK_SIZE 16

// Estructura de bloque de memoria
typedef struct heap_block {
    uint32_t size;              // Tamaño del bloque (incluyendo header)
    uint32_t used;              // 1 si está en uso, 0 si está libre
    struct heap_block* next;    // Siguiente bloque en la lista
    struct heap_block* prev;    // Bloque anterior en la lista
} __attribute__((packed)) heap_block_t;

// Funciones del heap
void heap_init(void);
void* kmalloc(uint32_t size);
void kfree(void* ptr);
uint32_t heap_get_free(void);
uint32_t heap_get_used(void);

#endif
