# Memory Management in HumanOS

## Overview

HumanOS implements a three-level memory management system:
1. **PMM (Physical Memory Manager)**: Physical page management using bitmap
2. **VMM (Virtual Memory Manager)**: Identity-mapped paging
3. **Heap**: Dynamic memory allocator for kernel

## 1. PMM - Physical Memory Manager

### Description

The PMM manages system physical memory using a bitmap where each bit represents a 4 KB page. Supports up to 4 GB of physical memory (1,048,576 pages).

### Data Structures

```c
typedef struct {
    uint32_t total_pages;      // Total pages in system
    uint32_t used_pages;       // Currently used pages
    uint32_t free_pages;       // Free pages available
    uint32_t* bitmap;          // Allocation bitmap (1 bit per page)
    uint32_t bitmap_size;      // Bitmap size in uint32_t
} pmm_t;
```

### Static Bitmap

The bitmap is stored statically in kernel memory, aligned to 4 KB:

```c
static uint32_t __attribute__((aligned(4096))) pmm_bitmap[32768];
```

- **Size**: 32768 uint32_t = 131,072 bytes = 128 KB
- **Capacity**: 32768 * 32 = 1,048,576 pages = 4 GB

### Initialization

#### pmm_init()

Basic initialization with fixed memory size:

```c
void pmm_init(uint32_t mem_size, uint32_t bitmap_addr);
```

Process:
1. Calculate total pages: `mem_size / PAGE_SIZE`
2. Calculate bitmap size: `(total_pages + 31) / 32`
3. Limit bitmap to 32768 entries (4 GB max)
4. Mark all pages as free (bitmap = 0)

#### pmm_init_multiboot()

Advanced initialization using Multiboot memory map:

```c
void pmm_init_multiboot(uint32_t mmap_addr, uint32_t mmap_length);
```

Process:
1. **Determine max usable memory**:
   - Iterate over type 1 entries (usable RAM)
   - Find highest accessible address
   - Limit to 0xFFFFFFFF (4 GB)

2. **Mark all as used**:
   - Initialize bitmap with all bits set to 1
   - `used_pages = total_pages`, `free_pages = 0`

3. **Free usable RAM regions**:
   - Iterate over type 1 entries
   - Mark corresponding pages as free
   - Update counters

4. **Protect first 2 MB**:
   - Mark pages 0-511 as used
   - Protects: Kernel, BIOS IVT/BDA, VGA buffer

### Operations

#### pmm_alloc_page()

Allocates a free physical page:

```c
void* pmm_alloc_page(void);
```

Process:
1. Check if free pages available
2. Find first uint32_t with free bit (not all 1)
3. Find first free bit in that uint32
4. Mark bit as used
5. Update counters
6. Return physical page address

Returns: `NULL` if out of memory, physical address if success

#### pmm_free_page()

Frees a physical page:

```c
void pmm_free_page(void* addr);
```

Process:
1. Verify 4 KB alignment
2. Calculate page index: `addr / PAGE_SIZE`
3. Calculate bitmap index: `page_index / 32`
4. Calculate bit: `1 << (page_index % 32)`
5. Verify double free (bit already free)
6. Mark bit as free
7. Update counters

## 2. VMM - Virtual Memory Manager

### Description

The VMM implements x86 paging with identity-mapping for the first 4 MB of memory. This allows virtual addresses to match physical addresses in this range.

### Data Structures

```c
typedef struct {
    uint32_t present    : 1;
    uint32_t rw         : 1;
    uint32_t user       : 1;
    uint32_t pwt        : 1;
    uint32_t pcd        : 1;
    uint32_t accessed   : 1;
    uint32_t dirty      : 1;
    uint32_t pat        : 1;
    uint32_t global     : 1;
    uint32_t available  : 3;
    uint32_t frame      : 20;
} page_entry_t;

typedef struct {
    page_entry_t entries[1024];
} page_table_t;

typedef struct {
    page_table_t* tables[1024];
    page_entry_t entries[1024];
} page_directory_t;
```

### Initialization

#### paging_init()

```c
void paging_init(void);
```

Process:
1. **Allocate page directory**:
   - Get page from PMM
   - Clear all entries

2. **Create page table for first 4 MB**:
   - Allocate table from PMM
   - Clear all entries

3. **Identity-map first 4 MB**:
   - For each page (0-1023):
     - `present = 1`, `rw = 1`, `user = 1`
     - `frame = physical_addr >> 12`
   - Allows user and supervisor access

4. **Configure directory entry**:
   - `present = 1`, `rw = 1`, `user = 1`
   - `frame = table_address >> 12`

5. **Load directory in CR3**:
   - `mov cr3, kernel_page_directory`

6. **Enable paging in CR0**:
   - Set PG bit (bit 31)

### Operations

#### page_map()

Maps virtual to physical address:

```c
void page_map(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
```

Flags:
- `PAGE_PRESENT`: Page present
- `PAGE_WRITE`: Read/write
- `PAGE_USER`: User access

#### page_unmap()

Unmaps virtual address:

```c
void page_unmap(uint32_t virtual_addr);
```

#### page_get_physical()

Gets physical address of virtual:

```c
uint32_t page_get_physical(uint32_t virtual_addr);
```

Returns: Physical address or 0 if not mapped

## 3. Heap - Dynamic Allocator

### Description

The heap provides dynamic memory allocation for the kernel using a first-fit algorithm with free block coalescing.

### Data Structures

```c
typedef struct heap_block {
    uint32_t size;              // Block size (including header)
    uint8_t used;               // 1 if used, 0 if free
    struct heap_block* next;     // Next block in list
    struct heap_block* prev;     // Previous block
} heap_block_t;
```

### Initialization

#### heap_init()

```c
void heap_init(void);
```

### Operations

#### kmalloc()

Allocates memory from heap:

```c
void* kmalloc(uint32_t size);
```

#### kfree()

Frees memory from heap:

```c
void kfree(void* ptr);
```

## Memory Layout

```
0x00000000 - 0x000FFFFF  (1 MB)    Kernel, BIOS, VGA
0x00100000 - 0x001FFFFF  (1 MB)    Extended kernel
0x00200000 - 0x003FFFFF  (2 MB)    Dynamic heap
0x00400000 - 0x00FFFFFF  (12 MB)   PMM bitmap, kernel structures
0x01000000 - 0xFFFFFFFF  (4 GB)    Remaining physical memory
```

## Configuration

- **PAGE_SIZE**: 4096 bytes (4 KB)
- **HEAP_START**: 0x00200000
- **HEAP_INITIAL_SIZE**: 1 page (4 KB)
- **MAX_MEMORY**: 4 GB (limited by bitmap)
