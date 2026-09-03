#include "paging.h"
#include "pmm.h"
#include "vga.h"
#include <stdint.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

// Directorio de páginas del kernel
static page_directory_t* kernel_page_directory = NULL;

// Tablas de páginas para los primeros 4 MB
static page_table_t* kernel_page_tables[1024];

// Inicializar paginación
void paging_init(void) {
    vga_puts("[PAGING] Initializing...\n", 0x1A);
    
    // Asignar directorio de páginas del PMM
    kernel_page_directory = (page_directory_t*)pmm_alloc_page();
    if (kernel_page_directory == NULL) {
        vga_puts("[PAGING] Failed to allocate page directory\n", 0x1C);
        return;
    }
    
    // Limpiar directorio de páginas
    for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) {
        kernel_page_directory->entries[i].present = 0;
        kernel_page_directory->entries[i].rw = 0;
        kernel_page_directory->entries[i].user = 0;
        kernel_page_directory->entries[i].pwt = 0;
        kernel_page_directory->entries[i].pcd = 0;
        kernel_page_directory->entries[i].accessed = 0;
        kernel_page_directory->entries[i].dirty = 0;
        kernel_page_directory->entries[i].pat = 0;
        kernel_page_directory->entries[i].global = 0;
        kernel_page_directory->entries[i].available = 0;
        kernel_page_directory->entries[i].frame = 0;
    }
    
    // Identity-mapear los primeros 4 MB (1024 páginas)
    // Usar 4 tablas de páginas (cada tabla mapea 4 MB)
    for (int dir_index = 0; dir_index < 1; dir_index++) {
        // Asignar tabla de páginas
        kernel_page_tables[dir_index] = (page_table_t*)pmm_alloc_page();
        if (kernel_page_tables[dir_index] == NULL) {
            vga_puts("[PAGING] Failed to allocate page table\n", 0x1C);
            return;
        }
        
        // Limpiar tabla de páginas
        for (int i = 0; i < PAGE_TABLE_ENTRIES; i++) {
            kernel_page_tables[dir_index]->entries[i].present = 0;
            kernel_page_tables[dir_index]->entries[i].rw = 0;
            kernel_page_tables[dir_index]->entries[i].user = 0;
            kernel_page_tables[dir_index]->entries[i].pwt = 0;
            kernel_page_tables[dir_index]->entries[i].pcd = 0;
            kernel_page_tables[dir_index]->entries[i].accessed = 0;
            kernel_page_tables[dir_index]->entries[i].dirty = 0;
            kernel_page_tables[dir_index]->entries[i].pat = 0;
            kernel_page_tables[dir_index]->entries[i].global = 0;
            kernel_page_tables[dir_index]->entries[i].available = 0;
            kernel_page_tables[dir_index]->entries[i].frame = 0;
        }
        
        // Mapear las primeras 1024 páginas (4 MB) identity-mapped permitiendo acceso de usuario (user = 1)
        for (int table_index = 0; table_index < 1024; table_index++) {
            uint32_t physical_addr = (dir_index * 1024 + table_index) * PAGE_SIZE;
            
            kernel_page_tables[dir_index]->entries[table_index].present = 1;
            kernel_page_tables[dir_index]->entries[table_index].rw = 1;    // Read/Write
            kernel_page_tables[dir_index]->entries[table_index].user = 1;  // User & Supervisor access
            kernel_page_tables[dir_index]->entries[table_index].frame = physical_addr >> 12;
        }
        
        // Configurar entrada en directorio de páginas (user = 1)
        kernel_page_directory->entries[dir_index].present = 1;
        kernel_page_directory->entries[dir_index].rw = 1;
        kernel_page_directory->entries[dir_index].user = 1;
        kernel_page_directory->entries[dir_index].frame = (uint32_t)kernel_page_tables[dir_index] >> 12;
    }
    
    // Cargar directorio de páginas en CR3
    __asm__ __volatile__("mov %0, %%cr3" : : "r"((uint32_t)kernel_page_directory));
    
    // Habilitar paginación en CR0
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;  // Set PG bit
    __asm__ __volatile__("mov %0, %%cr0" : : "r"(cr0));
    
    vga_puts("[PAGING] Enabled (identity-mapped first 4 MB)\n", 0x1A);
}

// Mapear dirección virtual a física
void page_map(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    uint32_t dir_index = virtual_addr >> 22;
    uint32_t table_index = (virtual_addr >> 12) & 0x3FF;
    
    // Verificar si existe la tabla de páginas
    if (kernel_page_directory->entries[dir_index].present == 0) {
        vga_puts("[PAGING] Page table not present\n", 0x1C);
        return;
    }
    
    page_table_t* table = (page_table_t*)(kernel_page_directory->entries[dir_index].frame << 12);
    
    // Mapear la página
    table->entries[table_index].present = (flags & PAGE_PRESENT) ? 1 : 0;
    table->entries[table_index].rw = (flags & PAGE_WRITE) ? 1 : 0;
    table->entries[table_index].user = (flags & PAGE_USER) ? 1 : 0;
    table->entries[table_index].frame = physical_addr >> 12;
    
    // Invalidar TLB
    __asm__ __volatile__("invlpg (%0)" : : "r"(virtual_addr));
}

// Desmapear dirección virtual
void page_unmap(uint32_t virtual_addr) {
    uint32_t dir_index = virtual_addr >> 22;
    uint32_t table_index = (virtual_addr >> 12) & 0x3FF;
    
    if (kernel_page_directory->entries[dir_index].present == 0) {
        return;
    }
    
    page_table_t* table = (page_table_t*)(kernel_page_directory->entries[dir_index].frame << 12);
    table->entries[table_index].present = 0;
    
    // Invalidar TLB
    __asm__ __volatile__("invlpg (%0)" : : "r"(virtual_addr));
}

// Obtener dirección física de dirección virtual
uint32_t page_get_physical(uint32_t virtual_addr) {
    uint32_t dir_index = virtual_addr >> 22;
    uint32_t table_index = (virtual_addr >> 12) & 0x3FF;
    uint32_t offset = virtual_addr & 0xFFF;
    
    if (kernel_page_directory->entries[dir_index].present == 0) {
        return 0;
    }
    
    page_table_t* table = (page_table_t*)(kernel_page_directory->entries[dir_index].frame << 12);
    
    if (table->entries[table_index].present == 0) {
        return 0;
    }
    
    return (table->entries[table_index].frame << 12) + offset;
}
