#include "heap.h"
#include "pmm.h"
#include "vga.h"
#include <stdint.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

// Heap global
static heap_block_t* heap_head = NULL;
static uint32_t heap_total = 0;
static uint32_t heap_used = 0;

// Inicializar heap
void heap_init(void) {
    heap_head = NULL;
    heap_total = 0;
    heap_used = 0;
    vga_puts("[HEAP] Initialized\n", 0x1A);
}

// Alinear tamaño a 4 bytes
static uint32_t align4(uint32_t size) {
    return (size + 3) & ~3;
}

// Crear nuevo bloque usando PMM
static heap_block_t* create_block(uint32_t size) {
    // Calcular tamaño total incluyendo header
    uint32_t total_size = align4(size) + sizeof(heap_block_t);
    
    // Calcular páginas necesarias
    uint32_t pages = (total_size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    // Asignar páginas del PMM
    void* page = pmm_alloc_page();
    if (page == NULL) {
        return NULL;
    }
    
    // Asignar páginas adicionales si es necesario
    for (uint32_t i = 1; i < pages; i++) {
        if (pmm_alloc_page() == NULL) {
            // Liberar páginas ya asignadas
            for (uint32_t j = 0; j < i; j++) {
                pmm_free_page((void*)((uint32_t)page + j * PAGE_SIZE));
            }
            return NULL;
        }
    }
    
    heap_block_t* block = (heap_block_t*)page;
    block->size = total_size;
    block->used = 1;
    block->next = NULL;
    block->prev = NULL;
    
    heap_total += total_size;
    heap_used += total_size;
    
    return block;
}

// Asignar memoria del heap
void* kmalloc(uint32_t size) {
    if (size == 0) {
        return NULL;
    }
    
    // Alinear tamaño
    uint32_t aligned_size = align4(size);
    
    // Buscar bloque libre que pueda contener el tamaño
    heap_block_t* block = heap_head;
    while (block != NULL) {
        if (!block->used && block->size >= aligned_size + sizeof(heap_block_t)) {
            // Bloque libre encontrado, usarlo
            block->used = 1;
            heap_used += block->size;
            return (void*)((uint32_t)block + sizeof(heap_block_t));
        }
        block = block->next;
    }
    
    // No se encontró bloque libre, crear nuevo
    heap_block_t* new_block = create_block(aligned_size);
    if (new_block == NULL) {
        return NULL;
    }
    
    // Agregar a la lista
    if (heap_head == NULL) {
        heap_head = new_block;
    } else {
        heap_block_t* last = heap_head;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = new_block;
        new_block->prev = last;
    }
    
    return (void*)((uint32_t)new_block + sizeof(heap_block_t));
}

// Liberar memoria del heap
void kfree(void* ptr) {
    if (ptr == NULL) {
        return;
    }
    
    heap_block_t* block = (heap_block_t*)((uint32_t)ptr - sizeof(heap_block_t));
    
    // Verificar si el bloque está en uso
    if (!block->used) {
        vga_puts("[HEAP] Double free detected\n", 0x1C);
        return;
    }
    
    block->used = 0;
    heap_used -= block->size;
    
    // Intentar fusionar con bloques adyacentes libres
    if (block->next != NULL && !block->next->used) {
        heap_block_t* next = block->next;
        block->size += next->size;
        block->next = next->next;
        if (next->next != NULL) {
            next->next->prev = block;
        }
        
        // Liberar páginas del bloque fusionado
        uint32_t pages = (next->size + PAGE_SIZE - 1) / PAGE_SIZE;
        for (uint32_t i = 0; i < pages; i++) {
            pmm_free_page((void*)((uint32_t)next + i * PAGE_SIZE));
        }
    }
    
    if (block->prev != NULL && !block->prev->used) {
        heap_block_t* prev = block->prev;
        prev->size += block->size;
        prev->next = block->next;
        if (block->next != NULL) {
            block->next->prev = prev;
        }
        
        // Liberar páginas del bloque fusionado
        uint32_t pages = (block->size + PAGE_SIZE - 1) / PAGE_SIZE;
        for (uint32_t i = 0; i < pages; i++) {
            pmm_free_page((void*)((uint32_t)block + i * PAGE_SIZE));
        }
    }
}

// Obtener memoria libre del heap
uint32_t heap_get_free(void) {
    return heap_total - heap_used;
}

// Obtener memoria usada del heap
uint32_t heap_get_used(void) {
    return heap_used;
}
