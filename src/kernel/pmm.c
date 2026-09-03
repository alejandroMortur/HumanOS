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

typedef struct {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed)) multiboot_memory_map_t;

// Inicializar PMM procesando dinámicamente el Mapa de Memoria de Multiboot
void pmm_init_multiboot(uint32_t mmap_addr, uint32_t mmap_length) {
    vga_puts("[PMM] Parsing Multiboot Memory Map...\n", 0x1A);
    
    // 1. Determinar el límite superior de memoria utilizable
    uint64_t max_usable_addr = 0;
    multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)mmap_addr;
    uint32_t mmap_end = mmap_addr + mmap_length;
    
    while ((uint32_t)mmap < mmap_end) {
        if (mmap->type == 1) { // Type 1 = RAM Utilizable
            uint64_t end_addr = mmap->addr + mmap->len;
            if (end_addr > max_usable_addr) {
                max_usable_addr = end_addr;
            }
        }
        mmap = (multiboot_memory_map_t*)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }
    
    if (max_usable_addr > 0xFFFFFFFF) {
        max_usable_addr = 0xFFFFFFFF;
    }
    
    uint32_t total_mem_bytes = (uint32_t)max_usable_addr;
    pmm.total_pages = total_mem_bytes / PAGE_SIZE;
    pmm.bitmap = pmm_bitmap;
    pmm.bitmap_size = (pmm.total_pages + 31) / 32;
    
    if (pmm.bitmap_size > 32768) {
        pmm.bitmap_size = 32768;
        pmm.total_pages = pmm.bitmap_size * 32;
    }
    
    // Marcar inicialmente TODAS las páginas como USADAS (reservadas)
    for (uint32_t i = 0; i < pmm.bitmap_size; i++) {
        pmm.bitmap[i] = 0xFFFFFFFF;
    }
    pmm.used_pages = pmm.total_pages;
    pmm.free_pages = 0;
    
    // 2. Marcar como LIBRES únicamente los bloques tipo 1 (RAM Utilizable)
    mmap = (multiboot_memory_map_t*)mmap_addr;
    while ((uint32_t)mmap < mmap_end) {
        if (mmap->type == 1) {
            uint32_t start_page = (uint32_t)(mmap->addr / PAGE_SIZE);
            uint32_t end_page = (uint32_t)((mmap->addr + mmap->len) / PAGE_SIZE);
            
            for (uint32_t page = start_page; page < end_page && page < pmm.total_pages; page++) {
                pmm_mark_free(page * PAGE_SIZE);
            }
        }
        mmap = (multiboot_memory_map_t*)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }
    
    // 3. Proteger los primeros 2 MB (Kernel, BIOS IVT/BDA, VGA buffer)
    for (uint32_t addr = 0; addr < 0x200000; addr += PAGE_SIZE) {
        pmm_mark_used(addr);
    }
    
    vga_puts("[PMM] Multiboot RAM detected: ", 0x1A);
    char buf[16];
    uint32_t mem_mb = (pmm.total_pages * PAGE_SIZE) / (1024 * 1024);
    int count = mem_mb;
    int j = 0;
    while (count > 0 && j < 15) {
        buf[j++] = '0' + (count % 10);
        count /= 10;
    }
    for (int k = j - 1; k >= 0; k--) vga_putc(buf[k], 0x1A);
    vga_puts(" MB, Free: ", 0x1A);
    
    count = pmm.free_pages;
    j = 0;
    while (count > 0 && j < 15) {
        buf[j++] = '0' + (count % 10);
        count /= 10;
    }
    for (int k = j - 1; k >= 0; k--) vga_putc(buf[k], 0x1A);
    vga_puts(" pages\n", 0x1A);
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
