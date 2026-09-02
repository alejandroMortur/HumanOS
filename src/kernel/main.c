#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "pmm.h"
#include "pit.h"
#include "heap.h"
#include "paging.h"
#include "scheduler.h"

static inline uint16_t read_ds(void) {
    uint16_t ds;
    __asm__ __volatile__("mov %%ds, %0" : "=r"(ds));
    return ds;
}

extern void keyboard_init(void);
extern void pit_init(uint32_t frequency);
extern void heap_init(void);
extern void paging_init(void);
extern void scheduler_init(void);

// Estructura de información de multiboot
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} __attribute__((packed)) multiboot_info_t;

void kernel_main(uint32_t magic, uint32_t mbi_addr) {
    vga_init();
    gdt_init();      // 1. Carga la GDT
    idt_init();      // 2. Carga la IDT en la CPU (lidt)
    
    // Verificar magic de multiboot
    if (magic != 0x2BADB002) {
        vga_puts("[ERROR] Not loaded by multiboot\n", 0x1C);
        // Usar valor por defecto de 128 MB
        pmm_init(128 * 1024 * 1024, 0x100000);
    } else {
        multiboot_info_t* mbi = (multiboot_info_t*)mbi_addr;
        
        // Calcular memoria total (mem_lower + mem_upper en KB)
        if (mbi->flags & 0x01) {
            uint32_t mem_total = (mbi->mem_lower + mbi->mem_upper) * 1024;
            pmm_init(mem_total, 0x100000);
            
            // Marcar las primeras páginas como usadas (kernel y datos)
            for (uint32_t addr = 0; addr < 0x200000; addr += PAGE_SIZE) {
                pmm_mark_used(addr);
            }
        } else {
            // Usar valor por defecto
            pmm_init(128 * 1024 * 1024, 0x100000);
            for (uint32_t addr = 0; addr < 0x200000; addr += PAGE_SIZE) {
                pmm_mark_used(addr);
            }
        }
    }
    
    // Inicializar PIT a 100 Hz
    pit_init(100);
    
    // Inicializar heap
    heap_init();
    
    // Inicializar paginación (DEBE ser antes de keyboard_init)
    paging_init();
    
    // Inicializar scheduler
    scheduler_init();
    
    keyboard_init(); // 3. Configura el PIC, la puerta 33 y hace sti

    vga_puts("======================================\n", 0x1E);
    vga_puts("  HumanOS v0.0.1 - Creado por Camila \n", 0x1E);
    vga_puts("======================================\n\n", 0x1E);

    uint16_t ds_val = read_ds();

    if (ds_val == 0x10) {
        vga_puts(" [OK] GDT cargada con exito en la CPU!\n", 0x1A);
        vga_puts(" [OK] Registro DS verificado: 0x10 (Data Segment)\n", 0x1A);
    } else {
        vga_puts(" [ERROR] La GDT no se ha configurado correctamente.\n", 0x1C);
    }

    // Texto usando ASCII estandar sin caracteres especiales
    vga_puts(" [OK] Driver de teclado activo. Escribe algo:\n\n> ", 0x1F);
    
    // Test PMM
    vga_puts("\n[PMM Test] Allocating 3 pages...\n", 0x1C);
    (void)pmm_alloc_page();
    void* page2 = pmm_alloc_page();
    (void)pmm_alloc_page();
    
    vga_puts("[PMM Test] Free pages: ", 0x1C);
    char buf[16];
    int count = pmm_get_free_pages();
    int j = 0;
    while (count > 0 && j < 15) {
        buf[j++] = '0' + (count % 10);
        count /= 10;
    }
    for (int k = j - 1; k >= 0; k--) {
        vga_putc(buf[k], 0x1C);
    }
    vga_putc('\n', 0x1C);
    
    vga_puts("[PMM Test] Freeing page2...\n", 0x1C);
    pmm_free_page(page2);
    
    vga_puts("[PMM Test] Free pages: ", 0x1C);
    count = pmm_get_free_pages();
    j = 0;
    while (count > 0 && j < 15) {
        buf[j++] = '0' + (count % 10);
        count /= 10;
    }
    for (int k = j - 1; k >= 0; k--) {
        vga_putc(buf[k], 0x1C);
    }
    vga_putc('\n', 0x1C);
    
    // Test Heap
    vga_puts("\n[HEAP Test] Allocating 100 bytes...\n", 0x1C);
    void* ptr1 = kmalloc(100);
    vga_puts("[HEAP Test] Used: ", 0x1C);
    count = heap_get_used();
    j = 0;
    while (count > 0 && j < 15) {
        buf[j++] = '0' + (count % 10);
        count /= 10;
    }
    for (int k = j - 1; k >= 0; k--) {
        vga_putc(buf[k], 0x1C);
    }
    vga_putc('\n', 0x1C);
    
    vga_puts("[HEAP Test] Freeing ptr1...\n", 0x1C);
    kfree(ptr1);
    
    vga_puts("[HEAP Test] Used: ", 0x1C);
    count = heap_get_used();
    j = 0;
    while (count > 0 && j < 15) {
        buf[j++] = '0' + (count % 10);
        count /= 10;
    }
    for (int k = j - 1; k >= 0; k--) {
        vga_putc(buf[k], 0x1C);
    }
    vga_putc('\n', 0x1C);
    
    // Proceso de prueba simple
    void test_process(void) {
        while (1) {
            for (volatile int i = 0; i < 10000000; i++);
            scheduler_yield();
        }
    }
    
    // Proceso idle del kernel
    void idle_process(void) {
        while (1) {
            for (volatile int i = 0; i < 5000000; i++);
            scheduler_yield();
        }
    }
    
    scheduler_convert_current_process("KernelMain");
    
    scheduler_add_process(idle_process, "IdleProcess");
    scheduler_add_process(test_process, "TestProcess");
    
    vga_puts("[PIT] Timer running at 100 Hz\n", 0x1A);
    vga_puts("[SCHEDULER] Processes added (preemptive multitasking)\n", 0x1C);
    
    // Loop principal - verificar schedule_needed y ceder control
    while (1) {
        scheduler_check();
        __asm__ __volatile__("hlt");
    }
}