#ifndef VFS_H
#define VFS_H

#include <stdint.h>

#define MAX_FILENAME 20
#define MAX_FILES 16
#define MAX_FILE_SIZE 1024

typedef struct {
    char name[MAX_FILENAME]; // 20 bytes
    uint32_t size;           // 4 bytes
    uint32_t start_lba;      // 4 bytes
    uint32_t used;           // 4 bytes
} __attribute__((packed)) vfs_file_t; // Total: 32 bytes x 16 = 512 bytes exactos

// Funciones del VFS y File System en Disco
void vfs_init(void);
int vfs_create_file(const char* name);
int vfs_write_file(const char* name, const char* content, uint32_t len);
int vfs_read_file(const char* name, char* buffer, uint32_t max_len);
void vfs_list_files(void);
int vfs_delete_file(const char* name);

#endif
