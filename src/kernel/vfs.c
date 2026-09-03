#include "vfs.h"
#include "ata.h"
#include "vga.h"
#include <stdint.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

#define VFS_METADATA_LBA 100
#define VFS_DATA_START_LBA 101

static vfs_file_t file_table[MAX_FILES];

static int strcmp_impl(const char* s1, const char* s2) {
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i]) return 1;
        i++;
    }
    return (s1[i] == s2[i]) ? 0 : 1;
}

static void strcpy_impl(char* dest, const char* src) {
    int i = 0;
    while (src[i] != '\0' && i < MAX_FILENAME - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

// Cargar tabla de archivos desde el sector 100 del disco duro (512 bytes exactos)
static void vfs_sync_from_disk(void) {
    uint8_t buf[512];
    if (ata_read_sector(VFS_METADATA_LBA, buf) == 0) {
        vfs_file_t* disk_files = (vfs_file_t*)buf;
        for (int i = 0; i < MAX_FILES; i++) {
            if (disk_files[i].used == 1 && disk_files[i].size <= MAX_FILE_SIZE) {
                file_table[i] = disk_files[i];
            } else {
                file_table[i].used = 0;
                file_table[i].size = 0;
                file_table[i].name[0] = '\0';
            }
        }
    }
}

// Guardar tabla de archivos en el sector 100 del disco duro (512 bytes exactos)
static void vfs_sync_to_disk(void) {
    uint8_t buf[512];
    vfs_file_t* disk_files = (vfs_file_t*)buf;
    for (int i = 0; i < MAX_FILES; i++) {
        disk_files[i] = file_table[i];
    }
    ata_write_sector(VFS_METADATA_LBA, buf);
}

void vfs_init(void) {
    for (int i = 0; i < MAX_FILES; i++) {
        file_table[i].used = 0;
        file_table[i].size = 0;
        file_table[i].start_lba = VFS_DATA_START_LBA + (i * 2); // 2 sectores (1024 B) por archivo
        file_table[i].name[0] = '\0';
    }
    
    // Cargar metadatos desde el disco duro ATA
    vfs_sync_from_disk();
    
    // Si la tabla está vacía en disco, crear un archivo de bienvenida inicial
    if (file_table[0].used == 0) {
        strcpy_impl(file_table[0].name, "readme.txt");
        file_table[0].used = 1;
        file_table[0].start_lba = VFS_DATA_START_LBA;
        
        const char* init_msg = "Welcome to HumanOS File System on ATA Disk!";
        int len = 0;
        while (init_msg[len] != '\0') len++;
        file_table[0].size = len + 1; // Include null terminator in size
        
        uint8_t sector_buf[512];
        for (int i = 0; i < 512; i++) sector_buf[i] = 0;
        for (int i = 0; i < len; i++) sector_buf[i] = (uint8_t)init_msg[i];
        sector_buf[len] = '\0'; // Add null terminator
        
        ata_write_sector(file_table[0].start_lba, sector_buf);
        vfs_sync_to_disk();
    }
    
    vga_puts("[VFS] Virtual File System Mounted on ATA Hard Disk (/)\n", COLOR_LIGHT_GREEN);
}

int vfs_create_file(const char* name) {
    if (name == NULL || name[0] == '\0') return -1;

    vfs_sync_from_disk();

    // Verificar si ya existe
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].used && strcmp_impl(file_table[i].name, name) == 0) {
            return i; // Ya existe
        }
    }

    // Buscar entrada libre
    for (int i = 0; i < MAX_FILES; i++) {
        if (!file_table[i].used) {
            file_table[i].used = 1;
            file_table[i].size = 0;
            file_table[i].start_lba = VFS_DATA_START_LBA + (i * 2);
            strcpy_impl(file_table[i].name, name);
            vfs_sync_to_disk();
            return i;
        }
    }

    return -1; // Sin espacio en directorio
}

int vfs_write_file(const char* name, const char* content, uint32_t len) {
    int idx = vfs_create_file(name);
    if (idx < 0) return -1;

    if (len > MAX_FILE_SIZE) len = MAX_FILE_SIZE;

    file_table[idx].size = len;

    // Escribir contenido en sectores ATA
    uint8_t buf[512];
    for (int i = 0; i < 512; i++) buf[i] = (i < (int)len) ? (uint8_t)content[i] : 0;
    ata_write_sector(file_table[idx].start_lba, buf);

    if (len > 512) {
        for (int i = 0; i < 512; i++) buf[i] = ((i + 512) < (int)len) ? (uint8_t)content[i + 512] : 0;
        ata_write_sector(file_table[idx].start_lba + 1, buf);
    }

    vfs_sync_to_disk();
    return len;
}

int vfs_read_file(const char* name, char* buffer, uint32_t max_len) {
    if (name == NULL || buffer == NULL || max_len == 0) return -1;

    vfs_sync_from_disk();

    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].used && strcmp_impl(file_table[i].name, name) == 0) {
            uint32_t bytes_to_read = file_table[i].size;
            if (bytes_to_read > max_len - 1) bytes_to_read = max_len - 1;

            uint8_t sector_buf[512];
            ata_read_sector(file_table[i].start_lba, sector_buf);

            for (uint32_t j = 0; j < bytes_to_read && j < 512; j++) {
                buffer[j] = (char)sector_buf[j];
            }

            if (bytes_to_read > 512) {
                ata_read_sector(file_table[i].start_lba + 1, sector_buf);
                for (uint32_t j = 512; j < bytes_to_read; j++) {
                    buffer[j] = (char)sector_buf[j - 512];
                }
            }

            buffer[bytes_to_read] = '\0';
            return bytes_to_read;
        }
    }

    return -1; // Archivo no encontrado
}

void vfs_list_files(void) {
    vfs_sync_from_disk();

    vga_puts("  Type      Size (Bytes)   Filename\n", COLOR_LIGHT_CYAN);
    vga_puts("  ----      ------------   --------\n", COLOR_LIGHT_CYAN);

    int count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].used) {
            count++;
            vga_puts("  FILE     ", COLOR_WHITE);

            char sbuf[16];
            int sz = file_table[i].size;
            int sj = 0;
            if (sz == 0) sbuf[sj++] = '0';
            while (sz > 0 && sj < 15) {
                sbuf[sj++] = '0' + (sz % 10);
                sz /= 10;
            }
            int padding = 12 - sj;
            for (int p = 0; p < padding; p++) vga_putc(' ', COLOR_WHITE);
            for (int k = sj - 1; k >= 0; k--) vga_putc(sbuf[k], COLOR_WHITE);

            vga_puts("   ", COLOR_WHITE);
            vga_puts(file_table[i].name, COLOR_LIGHT_GREEN);
            vga_putc('\n', COLOR_WHITE);
        }
    }

    if (count == 0) {
        vga_puts("  (No files stored on disk)\n", COLOR_WHITE);
    }
}

int vfs_delete_file(const char* name) {
    if (name == NULL || name[0] == '\0') return -1;

    vfs_sync_from_disk();

    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].used && strcmp_impl(file_table[i].name, name) == 0) {
            file_table[i].used = 0;
            file_table[i].size = 0;
            file_table[i].name[0] = '\0';
            vfs_sync_to_disk();
            return 0;
        }
    }

    return -1; // Archivo no encontrado
}
