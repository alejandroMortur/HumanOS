#include "pmm.h"
#include "vga.h"
#include <stdint.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

// Estructura PMM global
static pmm_t pmm;

// Bitmap estático (alineado a 4 KB) - 128 KB para hasta 4 GB de memoria
static uint32_t __attribute__((aligned(4096))) pmm_bitmap[32768];

// Inicializar PMM
void pmm_init(uint32_t mem_size, uint32_t __attribute__((unused)) bitmap_addr) {
    pmm.total_pages = mem_size / PAGE_SIZE;
    pmm.bitmap = pmm_bitmap;  // Usar bitmap estático en lugar de dirección externa
    pmm.bitmap_size = (pmm.total_pages + 31) / 32;
    
    // Limitar bitmap al tamaño disponible
    if (pmm.bitmap_size > 32768) {
        pmm.bitmap_size = 32768;
        pmm.total_pages = pmm.bitmap_size * 32;
    }
    
    pmm.used_pages = 0;
    pmm.free_pages = pmm.total_pages;
    
    // Limpiar bitmap (todas las páginas libres)
    for (uint32_t i = 0; i < pmm.bitmap_size; i++) {
        pmm.bitmap[i] = 0;
    }
    
    vga_puts("[PMM] Initialized with ", 0x1A);
    
    // Imprimir total de páginas
    char buf[16];
    int count = pmm.total_pages;
    int j = 0;
    while (count > 0 && j < 15) {
        buf[j++] = '0' + (count % 10);
        count /= 10;
    }
    for (int k = j - 1; k >= 0; k--) {
        vga_putc(buf[k], 0x1A);
    }
    
    vga_puts(" pages (", 0x1A);
    
    // Imprimir memoria total en MB
    uint32_t mem_mb = mem_size / (1024 * 1024);
    count = mem_mb;
    j = 0;
    while (count > 0 && j < 15) {
        buf[j++] = '0' + (count % 10);
        count /= 10;
    }
    for (int k = j - 1; k >= 0; k--) {
        vga_putc(buf[k], 0x1A);
    }
    
    vga_puts(" MB)\n", 0x1A);
}

// Asignar una página física
void* pmm_alloc_page(void) {
    if (pmm.free_pages == 0) {
        vga_puts("[PMM] Out of memory!\n", 0x1C);
        return NULL;
    }
    
    // Buscar el primer bit libre
    for (uint32_t i = 0; i < pmm.bitmap_size; i++) {
        if (pmm.bitmap[i] != 0xFFFFFFFF) {
            // Encontrar el primer bit libre en este uint32
            for (int j = 0; j < 32; j++) {
                uint32_t bit = 1 << j;
                if (!(pmm.bitmap[i] & bit)) {
                    // Marcar como usado
                    pmm.bitmap[i] |= bit;
                    pmm.used_pages++;
                    pmm.free_pages--;
                    
                    uint32_t page_index = i * 32 + j;
                    return (void*)(page_index * PAGE_SIZE);
                }
            }
        }
    }
    
    return NULL;
}

// Liberar una página física
void pmm_free_page(void* addr) {
    uint32_t page_addr = (uint32_t)addr;
    
    // Verificar alineación
    if (page_addr % PAGE_SIZE != 0) {
        vga_puts("[PMM] Invalid address alignment\n", 0x1C);
        return;
    }
    
    uint32_t page_index = page_addr / PAGE_SIZE;
    uint32_t bitmap_index = page_index / 32;
    uint32_t bit = 1 << (page_index % 32);
    
    // Verificar si ya está libre
    if (!(pmm.bitmap[bitmap_index] & bit)) {
        vga_puts("[PMM] Double free detected\n", 0x1C);
        return;
    }
    
    // Marcar como libre
    pmm.bitmap[bitmap_index] &= ~bit;
    pmm.used_pages--;
    pmm.free_pages++;
}

// Obtener páginas libres
uint32_t pmm_get_free_pages(void) {
    return pmm.free_pages;
}

// Obtener páginas usadas
uint32_t pmm_get_used_pages(void) {
    return pmm.used_pages;
}

// Marcar rango de memoria como usado
void pmm_mark_used(uint32_t addr) {
    uint32_t page_index = addr / PAGE_SIZE;
    uint32_t bitmap_index = page_index / 32;
    uint32_t bit = 1 << (page_index % 32);
    
    if (!(pmm.bitmap[bitmap_index] & bit)) {
        pmm.bitmap[bitmap_index] |= bit;
        pmm.used_pages++;
        pmm.free_pages--;
    }
}

// Marcar rango de memoria como libre
void pmm_mark_free(uint32_t addr) {
    uint32_t page_index = addr / PAGE_SIZE;
    uint32_t bitmap_index = page_index / 32;
    uint32_t bit = 1 << (page_index % 32);
    
    if (pmm.bitmap[bitmap_index] & bit) {
        pmm.bitmap[bitmap_index] &= ~bit;
        pmm.used_pages--;
        pmm.free_pages++;
    }
}
