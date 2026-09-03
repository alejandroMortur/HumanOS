#include "elf.h"
#include "vfs.h"
#include "heap.h"
#include "vga.h"
#include "scheduler.h"
#include <stdint.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

// Validar cabecera de binario ELF 32-bit (x86)
int elf_validate(const Elf32_Ehdr* header) {
    if (header == NULL) return 0;

    // Verificar número mágico \x7FELF
    uint32_t* magic = (uint32_t*)header->e_ident;
    if (*magic != ELF_MAGIC) {
        return 0; // No es un binario ELF
    }

    // Verificar arquitectura x86 32-bit (EM_386)
    if (header->e_machine != EM_386) {
        return 0; // Arquitectura no compatible
    }

    // Verificar que sea ejecutable (ET_EXEC) o relocalizable (ET_REL)
    if (header->e_type != ET_EXEC && header->e_type != ET_REL) {
        return 0;
    }

    return 1;
}

// Cargar binario ELF desde el sistema de archivos VFS a memoria
int elf_load_from_vfs(const char* filename, void** entry_point) {
    if (filename == NULL || entry_point == NULL) return -1;

    // Leer encabezado ELF desde disco VFS
    uint8_t buffer[1024];
    int read_bytes = vfs_read_file(filename, (char*)buffer, sizeof(buffer));
    if (read_bytes < (int)sizeof(Elf32_Ehdr)) {
        vga_puts("[ELF] Error: File too small or not found: ", COLOR_LIGHT_RED);
        vga_puts(filename, COLOR_WHITE);
        vga_putc('\n', COLOR_WHITE);
        return -1;
    }

    Elf32_Ehdr* header = (Elf32_Ehdr*)buffer;

    // Validar cabecera ELF
    if (!elf_validate(header)) {
        vga_puts("[ELF] Error: Invalid 32-bit x86 ELF format: ", COLOR_LIGHT_RED);
        vga_puts(filename, COLOR_WHITE);
        vga_putc('\n', COLOR_WHITE);
        return -1;
    }

    // Extraer punto de entrada
    *entry_point = (void*)header->e_entry;

    // Mapear segmentos de programa (PT_LOAD)
    Elf32_Phdr* phdr = (Elf32_Phdr*)(buffer + header->e_phoff);
    for (int i = 0; i < header->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            // Asignar memoria para el segmento ejecutable
            void* seg_mem = kmalloc(phdr[i].p_memsz > 0 ? phdr[i].p_memsz : 512);
            if (seg_mem == NULL) {
                vga_puts("[ELF] Error: Out of memory loading segment\n", COLOR_LIGHT_RED);
                return -1;
            }

            // Copiar contenido del segmento desde el archivo
            uint8_t* src = buffer + phdr[i].p_offset;
            uint8_t* dest = (uint8_t*)seg_mem;

            for (uint32_t k = 0; k < phdr[i].p_filesz && k < (uint32_t)read_bytes; k++) {
                dest[k] = src[k];
            }

            // Inicializar sección .bss a cero (memsz > filesz)
            for (uint32_t k = phdr[i].p_filesz; k < phdr[i].p_memsz; k++) {
                dest[k] = 0;
            }

            // Actualizar punto de entrada si apunta a memoria del segmento asignado
            if (header->e_entry >= phdr[i].p_vaddr && header->e_entry < phdr[i].p_vaddr + phdr[i].p_memsz) {
                uint32_t offset = header->e_entry - phdr[i].p_vaddr;
                *entry_point = (void*)((uint32_t)seg_mem + offset);
            }
        }
    }

    vga_puts("[ELF] Successfully Loaded Binary: ", COLOR_LIGHT_GREEN);
    vga_puts(filename, COLOR_WHITE);
    vga_putc('\n', COLOR_WHITE);

    return 0;
}

// Cargar y ejecutar binario ELF como proceso de usuario Ring 3
int elf_run(const char* filename) {
    void* entry_point = NULL;
    if (elf_load_from_vfs(filename, &entry_point) == 0 && entry_point != NULL) {
        scheduler_add_user_process((void(*)(void))entry_point, filename);
        return 0;
    }
    return -1;
}
