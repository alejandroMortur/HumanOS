# Sistema de Archivos (VFS) en HumanOS

## Visión General

HumanOS implementa un Virtual File System (VFS) simple que almacena archivos en disco duro ATA. El sistema usa un diseño flat (sin directorios) con metadatos en un sector fijo y datos en sectores consecutivos.

## Estructuras de Datos

### vfs_file_t

```c
typedef struct {
    char name[MAX_FILENAME];    // Nombre del archivo (32 caracteres)
    uint32_t size;             // Tamaño en bytes
    uint32_t start_lba;        // Sector inicial en disco
    uint8_t used;              // 1 si archivo existe, 0 si libre
} vfs_file_t;
```

### Constantes

```c
#define MAX_FILES 64            // Máximo de archivos
#define MAX_FILENAME 32         // Máximo longitud de nombre
#define MAX_FILE_SIZE 1024      // Máximo tamaño por archivo (1 KB)
#define VFS_METADATA_LBA 100    // Sector de metadatos
#define VFS_DATA_START_LBA 101 // Primer sector de datos
```

## Layout del Disco

```
Sector 0-99:    Reservado (kernel, bootloader, etc.)
Sector 100:     Metadatos VFS (file_table)
Sector 101-102:  Archivo 0 (2 sectores = 1 KB)
Sector 103-104:  Archivo 1 (2 sectores = 1 KB)
...
Sector 227-228:  Archivo 63 (2 sectores = 1 KB)
Sector 229+:    Espacio libre
```

## Operaciones del VFS

### vfs_create_file()

```c
int vfs_create_file(const char* name);
```

Crea un archivo nuevo o retorna índice si ya existe.

### vfs_write_file()

```c
int vfs_write_file(const char* name, const char* content, uint32_t len);
```

Escribe contenido a un archivo.

### vfs_read_file()

```c
int vfs_read_file(const char* name, char* buffer, uint32_t max_len);
```

Lee contenido de un archivo.

### vfs_list_files()

```c
void vfs_list_files(void);
```

Lista todos los archivos en el sistema.

## Sincronización con Disco

### vfs_sync_from_disk()

Lee metadatos del sector 100 del disco ATA.

### vfs_sync_to_disk()

Escribe metadatos al sector 100 del disco ATA.
