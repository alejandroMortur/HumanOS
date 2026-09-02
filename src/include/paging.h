#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

// Tamaño de página: 4 KB
#define PAGE_SIZE 4096

// Número de entradas en directorio/tabla
#define PAGE_TABLE_ENTRIES 1024

// Flags de entrada de página
#define PAGE_PRESENT    0x001  // P: Present
#define PAGE_WRITE      0x002  // R/W: Read/Write
#define PAGE_USER       0x004  // U/S: User/Supervisor
#define PAGE_WRITETHROUGH 0x008  // PWT: Write-Through
#define PAGE_NOCACHE    0x010  // PCD: Cache-Disabled
#define PAGE_ACCESSED   0x020  // A: Accessed
#define PAGE_DIRTY      0x040  // D: Dirty
#define PAGE_GLOBAL     0x100  // G: Global
#define PAGE_FRAME      0xFFFFF000  // Frame address (bits 12-31)

// Estructura de entrada de página
typedef struct {
    uint32_t present    : 1;
    uint32_t rw         : 1;  // Read/Write
    uint32_t user       : 1;  // User/Supervisor
    uint32_t pwt        : 1;  // Page-Level Write-Through
    uint32_t pcd        : 1;  // Page-Level Cache-Disable
    uint32_t accessed   : 1;
    uint32_t dirty      : 1;
    uint32_t pat        : 1;  // Page Attribute Table
    uint32_t global     : 1;
    uint32_t available  : 3;
    uint32_t frame      : 20; // Frame address (bits 12-31)
} __attribute__((packed)) page_entry_t;

// Estructura de directorio de páginas
typedef struct {
    page_entry_t entries[PAGE_TABLE_ENTRIES];
} __attribute__((aligned(PAGE_SIZE))) page_directory_t;

// Estructura de tabla de páginas
typedef struct {
    page_entry_t entries[PAGE_TABLE_ENTRIES];
} __attribute__((aligned(PAGE_SIZE))) page_table_t;

// Funciones de paginación
void paging_init(void);
void page_map(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
void page_unmap(uint32_t virtual_addr);
uint32_t page_get_physical(uint32_t virtual_addr);

#endif
