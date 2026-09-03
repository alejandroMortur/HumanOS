# File System (VFS) in HumanOS

## Overview

HumanOS implements a Virtual File System (VFS) that stores files on ATA hard disk. The system uses a flat design (no directories) with metadata in a fixed sector and data in consecutive sectors.

## Data Structures

### vfs_file_t

```c
typedef struct {
    char name[MAX_FILENAME];    // File name (32 characters)
    uint32_t size;             // Size in bytes
    uint32_t start_lba;        // Starting sector on disk
    uint8_t used;              // 1 if file exists, 0 if free
} vfs_file_t;
```

### Constants

```c
#define MAX_FILES 64            // Maximum files
#define MAX_FILENAME 32         // Maximum name length
#define MAX_FILE_SIZE 1024      // Maximum size per file (1 KB)
#define VFS_METADATA_LBA 100    // Metadata sector
#define VFS_DATA_START_LBA 101 // First data sector
```

## Disk Layout

```
Sector 0-99:    Reserved (kernel, bootloader, etc.)
Sector 100:     VFS metadata (file_table)
Sector 101-102:  File 0 (2 sectors = 1 KB)
Sector 103-104:  File 1 (2 sectors = 1 KB)
...
Sector 227-228:  File 63 (2 sectors = 1 KB)
Sector 229+:    Free space
```

## VFS Operations

### vfs_create_file()

```c
int vfs_create_file(const char* name);
```

Creates a new file or returns index if already exists.

### vfs_write_file()

```c
int vfs_write_file(const char* name, const char* content, uint32_t len);
```

Writes content to a file.

### vfs_read_file()

```c
int vfs_read_file(const char* name, char* buffer, uint32_t max_len);
```

Reads content from a file.

### vfs_list_files()

```c
void vfs_list_files(void);
```

Lists all files in the system.

## Disk Synchronization

### vfs_sync_from_disk()

Reads metadata from sector 100 of ATA disk.

### vfs_sync_to_disk()

Writes metadata to sector 100 of ATA disk.
