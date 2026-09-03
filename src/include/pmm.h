#ifndef PMM_H
#define PMM_H

#include <stdint.h>

// Tamaño de página: 4 KB
#define PAGE_SIZE 4096
#define PAGE_ALIGN(addr) (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

// Estructura PMM
typedef struct {
    uint32_t* bitmap;           // Bitmap de páginas (1 bit = 1 página)
    uint32_t total_pages;       // Total de páginas disponibles
    uint32_t used_pages;         // Páginas en uso
    uint32_t free_pages;         // Páginas libres
    uint32_t bitmap_size;       // Tamaño del bitmap en bytes
} pmm_t;

// Funciones PMM
void pmm_init(uint32_t mem_size, uint32_t bitmap_addr);
void pmm_init_multiboot(uint32_t mmap_addr, uint32_t mmap_length);
void* pmm_alloc_page(void);
void pmm_free_page(void* addr);
uint32_t pmm_get_free_pages(void);
uint32_t pmm_get_used_pages(void);
void pmm_mark_used(uint32_t addr);
void pmm_mark_free(uint32_t addr);

#endif
