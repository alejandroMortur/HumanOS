#ifndef ELF_H
#define ELF_H

#include <stdint.h>

#define ELF_MAGIC 0x464C457F // "\x7FELF" en little endian

#define ET_NONE 0
#define ET_REL  1
#define ET_EXEC 2
#define ET_DYN  3

#define EM_386  3 // x86 / i386 Architecture

#define PT_NULL 0
#define PT_LOAD 1 // Loadable Segment

#define PF_X 1 // Executable
#define PF_W 2 // Writable
#define PF_R 4 // Readable

typedef struct {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;     // Punto de entrada virtual
    uint32_t e_phoff;     // Offset de la tabla de cabeceras de programa
    uint32_t e_shoff;     // Offset de la tabla de cabeceras de sección
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;     // Número de cabeceras de programa
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) Elf32_Ehdr;

typedef struct {
    uint32_t p_type;     // Tipo de segmento (PT_LOAD)
    uint32_t p_offset;   // Offset en el archivo
    uint32_t p_vaddr;    // Dirección virtual de destino
    uint32_t p_paddr;
    uint32_t p_filesz;   // Tamaño en archivo
    uint32_t p_memsz;    // Tamaño en memoria (.bss si memsz > filesz)
    uint32_t p_flags;    // Permisos (PF_R, PF_W, PF_X)
    uint32_t p_align;
} __attribute__((packed)) Elf32_Phdr;

// Prototipos del Cargador ELF
int elf_validate(const Elf32_Ehdr* header);
int elf_load_from_vfs(const char* filename, void** entry_point);
int elf_run(const char* filename);

#endif
