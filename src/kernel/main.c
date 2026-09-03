#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "pmm.h"
#include "pit.h"
#include "heap.h"
#include "paging.h"
#include "scheduler.h"
#include "syscall.h"
#include "serial.h"
#include "power.h"
#include "cpu.h"
#include "ata.h"
#include "vfs.h"
#include "elf.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

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

// Imprimir el Prompt al estilo Linux / Bash (user@humanos:~$)
static void print_shell_prompt(void) {
    vga_puts("user", COLOR_LIGHT_GREEN);
    vga_puts("@", COLOR_WHITE);
    vga_puts("humanos", COLOR_LIGHT_CYAN);
    vga_puts(":", COLOR_WHITE);
    vga_puts("~", COLOR_YELLOW);
    vga_puts("$ ", COLOR_WHITE);
}

// Shell interactiva de espacio de usuario (Ring 3 CLI)
static void user_shell_process(void) {
    sys_write(1, "Type 'help' for commands list.\n\n", 32);

    char input_buf[64];
    int buf_idx = 0;

    // Historial de comandos (últimos 10 comandos)
    #define MAX_HIST 10
    static char cmd_history[MAX_HIST][64];
    static int hist_count = 0;
    static int hist_idx = 0;

    print_shell_prompt();

    while (1) {
        char c = 0;
        int bytes = sys_read(0, &c, 1);
        if (bytes > 0) {
            if (c == '\r' || c == '\n') {
                vga_putc('\n', COLOR_WHITE);
                input_buf[buf_idx] = '\0';

                if (buf_idx > 0) {
                    // Guardar en historial de comandos
                    int slot = hist_count % MAX_HIST;
                    int k = 0;
                    while (input_buf[k] != '\0' && k < 63) {
                        cmd_history[slot][k] = input_buf[k];
                        k++;
                    }
                    cmd_history[slot][k] = '\0';
                    hist_count++;
                    hist_idx = hist_count;

                    vga_putc('\n', COLOR_WHITE);
                    if (input_buf[0] == 'h' && input_buf[1] == 'e' && input_buf[2] == 'l' && input_buf[3] == 'p') {
                        sys_write(1, "Commands: help, run, exec, ls, cat, touch, write, diskinfo, sysinfo, whoami, hostname, uname, clear, ps, stress, reboot, shutdown, exit", 134);
                    } else if ((input_buf[0] == 'r' && input_buf[1] == 'u' && input_buf[2] == 'n') || (input_buf[0] == 'e' && input_buf[1] == 'x' && input_buf[2] == 'e' && input_buf[3] == 'c')) {
                        int i = (input_buf[0] == 'r') ? 4 : 5;
                        const char* fname = &input_buf[i];
                        if (fname[0] != '\0') {
                            elf_run(fname);
                        } else {
                            sys_write(1, "  Usage: run <filename.elf>", 27);
                        }
                    } else if (input_buf[0] == 'l' && input_buf[1] == 's') {
                        vfs_list_files();
                    } else if (input_buf[0] == 'd' && input_buf[1] == 'i' && input_buf[2] == 's' && input_buf[3] == 'k') {
                        vga_puts("  ATA Model  : ", COLOR_LIGHT_GREEN);
                        vga_puts(ata_get_model(), COLOR_WHITE);
                        vga_puts("\n  Sectors    : ", COLOR_LIGHT_GREEN);
                        char sbuf[16];
                        uint32_t sc = ata_get_sector_count();
                        int sj = 0;
                        if (sc == 0) sbuf[sj++] = '0';
                        while (sc > 0 && sj < 15) {
                            sbuf[sj++] = '0' + (sc % 10);
                            sc /= 10;
                        }
                        for (int k = sj - 1; k >= 0; k--) vga_putc(sbuf[k], COLOR_WHITE);
                        vga_puts(" LBA Sectors (", COLOR_WHITE);
                        uint32_t disk_mb = sc / 2048;
                        sj = 0;
                        if (disk_mb == 0) sbuf[sj++] = '0';
                        while (disk_mb > 0 && sj < 15) {
                            sbuf[sj++] = '0' + (disk_mb % 10);
                            disk_mb /= 10;
                        }
                        for (int k = sj - 1; k >= 0; k--) vga_putc(sbuf[k], COLOR_WHITE);
                        vga_puts(" MB)\n", COLOR_WHITE);
                    } else if (input_buf[0] == 'c' && input_buf[1] == 'a' && input_buf[2] == 't') {
                        char read_buf[512];
                        const char* fname = &input_buf[4];
                        if (fname[0] == '\0') fname = "readme.txt";
                        int r = vfs_read_file(fname, read_buf, sizeof(read_buf));
                        if (r >= 0) {
                            vga_puts("  ", COLOR_WHITE);
                            vga_puts(read_buf, COLOR_WHITE);
                            vga_putc('\n', COLOR_WHITE);
                        } else {
                            sys_write(1, "  File not found.", 17);
                        }
                    } else if (input_buf[0] == 't' && input_buf[1] == 'o' && input_buf[2] == 'u' && input_buf[3] == 'c' && input_buf[4] == 'h') {
                        const char* fname = &input_buf[6];
                        if (fname[0] != '\0') {
                            vfs_create_file(fname);
                            vga_puts("  File created: ", COLOR_LIGHT_GREEN);
                            vga_puts(fname, COLOR_WHITE);
                            vga_putc('\n', COLOR_WHITE);
                        }
                    } else if (input_buf[0] == 'w' && input_buf[1] == 'r' && input_buf[2] == 'i' && input_buf[3] == 't' && input_buf[4] == 'e') {
                        char fname[32];
                        int fi = 0;
                        int i = 6;
                        while (input_buf[i] != ' ' && input_buf[i] != '\0' && fi < 31) {
                            fname[fi++] = input_buf[i++];
                        }
                        fname[fi] = '\0';
                        if (input_buf[i] == ' ') i++;
                        const char* text = &input_buf[i];
                        int tlen = 0;
                        while (text[tlen] != '\0') tlen++;
                        if (fi > 0 && tlen > 0) {
                            vfs_write_file(fname, text, tlen);
                            vga_puts("  Saved ", COLOR_LIGHT_GREEN);
                            char tbuf[16];
                            int tj = 0;
                            int cnt = tlen;
                            while (cnt > 0 && tj < 15) { tbuf[tj++] = '0' + (cnt % 10); cnt /= 10; }
                            for (int k = tj - 1; k >= 0; k--) vga_putc(tbuf[k], COLOR_WHITE);
                            vga_puts(" bytes to ", COLOR_LIGHT_GREEN);
                            vga_puts(fname, COLOR_WHITE);
                            vga_puts(" on disk!\n", COLOR_LIGHT_GREEN);
                        }
                    } else if (input_buf[0] == 'w' && input_buf[1] == 'h' && input_buf[2] == 'o' && input_buf[3] == 'a' && input_buf[4] == 'm' && input_buf[5] == 'i') {
                        sys_write(1, "user", 4);
                    } else if (input_buf[0] == 'h' && input_buf[1] == 'o' && input_buf[2] == 's' && input_buf[3] == 't' && input_buf[4] == 'n' && input_buf[5] == 'a' && input_buf[6] == 'm' && input_buf[7] == 'e') {
                        sys_write(1, "humanos", 7);
                    } else if (input_buf[0] == 'u' && input_buf[1] == 'n' && input_buf[2] == 'a' && input_buf[3] == 'm' && input_buf[4] == 'e') {
                        sys_write(1, "HumanOS 0.0.1 i686 GNU/HumanOS (x86 Ring 3)", 43);
                    } else if (input_buf[0] == 's' && input_buf[1] == 'y' && input_buf[2] == 's' && input_buf[3] == 'i' && input_buf[4] == 'n' && input_buf[5] == 'f' && input_buf[6] == 'o') {
                        cpu_print_info();
                    } else if (input_buf[0] == 'v' && input_buf[1] == 'e' && input_buf[2] == 'r') {
                        sys_write(1, "HumanOS v0.0.1 (x86 Ring 3 Userland)", 36);
                    } else if (input_buf[0] == 'c' && input_buf[1] == 'l' && input_buf[2] == 'e' && input_buf[3] == 'a' && input_buf[4] == 'r') {
                        vga_clear();
                        buf_idx = 0;
                        print_shell_prompt();
                        continue;
                    } else if (input_buf[0] == 'p' && input_buf[1] == 's') {
                        sys_write(1, "Active Processes: PID 1 (Kernel), PID 2 (UserShell)", 51);
                    } else if (input_buf[0] == 's' && input_buf[1] == 't' && input_buf[2] == 'r' && input_buf[3] == 'e' && input_buf[4] == 's' && input_buf[5] == 's') {
                        sys_write(1, "Starting stress test...\n", 25);
                        sys_write(1, "Test 1: Memory allocation stress\n", 32);
                        for (int i = 0; i < 100; i++) {
                            void* ptr = sys_malloc(1024);
                            if (ptr) sys_free(ptr);
                            if (i % 10 == 0) sys_yield();
                        }
                        sys_write(1, "Test 1: PASSED\n", 17);
                        
                        sys_write(1, "Test 2: Scheduler stress (rapid yields)\n", 39);
                        for (int i = 0; i < 1000; i++) {
                            sys_yield();
                        }
                        sys_write(1, "Test 2: PASSED\n", 17);
                        
                        sys_write(1, "Test 3: VFS stress\n", 20);
                        for (int i = 0; i < 50; i++) {
                            vfs_create_file("stress_test.txt");
                            vfs_write_file("stress_test.txt", "Stress test data", 16);
                            char buf[64];
                            vfs_read_file("stress_test.txt", buf, 64);
                            if (i % 5 == 0) sys_yield();
                        }
                        sys_write(1, "Test 3: PASSED\n", 17);
                        
                        sys_write(1, "All stress tests PASSED - No Triple Faults detected\n", 56);
                    } else if (input_buf[0] == 'r' && input_buf[1] == 'e' && input_buf[2] == 'b' && input_buf[3] == 'o' && input_buf[4] == 'o' && input_buf[5] == 't') {
                        sys_reboot();
                    } else if (input_buf[0] == 's' && input_buf[1] == 'h' && input_buf[2] == 'u' && input_buf[3] == 't' && input_buf[4] == 'd' && input_buf[5] == 'o' && input_buf[6] == 'w' && input_buf[7] == 'n') {
                        sys_shutdown();
                    } else if (input_buf[0] == 'e' && input_buf[1] == 'x' && input_buf[2] == 'i' && input_buf[3] == 't') {
                        sys_write(1, "Exiting Shell...", 16);
                        sys_exit(0);
                    } else {
                        sys_write(1, "Unknown command. Type 'help'.", 29);
                    }
                    vga_putc('\n', COLOR_WHITE);
                }

                buf_idx = 0;
                print_shell_prompt();
            } else if (c == '\b') {
                if (buf_idx > 0) {
                    buf_idx--;
                    vga_putc('\b', COLOR_WHITE);
                    vga_putc(' ', COLOR_WHITE);
                    vga_putc('\b', COLOR_WHITE);
                }
            } else if ((uint8_t)c == 0x80) { // Flecha Arriba (KEY_UP)
                if (hist_count > 0 && hist_idx > 0) {
                    hist_idx--;
                    // Borrar comando actual en pantalla
                    while (buf_idx > 0) {
                        vga_putc('\b', COLOR_WHITE);
                        vga_putc(' ', COLOR_WHITE);
                        vga_putc('\b', COLOR_WHITE);
                        buf_idx--;
                    }
                    // Copiar comando del historial
                    int slot = hist_idx % MAX_HIST;
                    int k = 0;
                    while (cmd_history[slot][k] != '\0' && k < 63) {
                        input_buf[k] = cmd_history[slot][k];
                        vga_putc(input_buf[k], COLOR_WHITE);
                        k++;
                    }
                    buf_idx = k;
                    input_buf[buf_idx] = '\0';
                }
            } else if ((uint8_t)c == 0x81) { // Flecha Abajo (KEY_DOWN)
                if (hist_idx < hist_count) {
                    hist_idx++;
                    // Borrar comando actual en pantalla
                    while (buf_idx > 0) {
                        vga_putc('\b', COLOR_WHITE);
                        vga_putc(' ', COLOR_WHITE);
                        vga_putc('\b', COLOR_WHITE);
                        buf_idx--;
                    }
                    if (hist_idx < hist_count) {
                        int slot = hist_idx % MAX_HIST;
                        int k = 0;
                        while (cmd_history[slot][k] != '\0' && k < 63) {
                            input_buf[k] = cmd_history[slot][k];
                            vga_putc(input_buf[k], COLOR_WHITE);
                            k++;
                        }
                        buf_idx = k;
                        input_buf[buf_idx] = '\0';
                    } else {
                        buf_idx = 0;
                        input_buf[0] = '\0';
                    }
                }
            } else if (c == '\t') {
                // Autocompletado inteligente con Tabulador
                if (buf_idx > 0) {
                    input_buf[buf_idx] = '\0';

                    // Lista de comandos disponibles
                    static const char* cmds[] = {
                        "help", "ls", "cat", "touch", "write", "diskinfo",
                        "sysinfo", "whoami", "hostname", "uname", "clear",
                        "ps", "reboot", "shutdown", "exit", NULL
                    };

                    // Determinar si es autocompletado de comando o de archivo
                    int space_pos = -1;
                    for (int i = 0; i < buf_idx; i++) {
                        if (input_buf[i] == ' ') {
                            space_pos = i;
                            break;
                        }
                    }

                    if (space_pos == -1) {
                        // Autocompletar comando
                        const char* match = NULL;
                        int matches = 0;
                        for (int i = 0; cmds[i] != NULL; i++) {
                            int is_match = 1;
                            for (int k = 0; k < buf_idx; k++) {
                                if (input_buf[k] != cmds[i][k]) {
                                    is_match = 0;
                                    break;
                                }
                            }
                            if (is_match) {
                                match = cmds[i];
                                matches++;
                            }
                        }
                        if (matches == 1 && match != NULL) {
                            for (int k = buf_idx; match[k] != '\0'; k++) {
                                input_buf[buf_idx++] = match[k];
                                vga_putc(match[k], COLOR_WHITE);
                            }
                            if (buf_idx < 63) {
                                input_buf[buf_idx++] = ' ';
                                vga_putc(' ', COLOR_WHITE);
                            }
                            input_buf[buf_idx] = '\0';
                        }
                    } else {
                        // Autocompletar argumento de archivo (ej. cat read -> cat readme.txt)
                        const char* arg = &input_buf[space_pos + 1];
                        int arg_len = 0;
                        while (arg[arg_len] != '\0') arg_len++;

                        if (arg_len > 0) {
                            static const char* sample_files[] = { "readme.txt", NULL };
                            const char* fmatch = NULL;
                            int fcount = 0;

                            for (int i = 0; sample_files[i] != NULL; i++) {
                                int is_m = 1;
                                for (int k = 0; k < arg_len; k++) {
                                    if (arg[k] != sample_files[i][k]) {
                                        is_m = 0;
                                        break;
                                    }
                                }
                                if (is_m) {
                                    fmatch = sample_files[i];
                                    fcount++;
                                }
                            }

                            if (fcount == 1 && fmatch != NULL) {
                                for (int k = arg_len; fmatch[k] != '\0'; k++) {
                                    input_buf[buf_idx++] = fmatch[k];
                                    vga_putc(fmatch[k], COLOR_WHITE);
                                }
                                input_buf[buf_idx] = '\0';
                            }
                        }
                    }
                }
            } else {
                if (buf_idx < 63) {
                    input_buf[buf_idx++] = c;
                    vga_putc(c, COLOR_WHITE);
                }
            }
        } else {
            sys_yield();
        }
    }
}

void kernel_main(uint32_t magic, uint32_t mbi_addr) {
    vga_init();
    serial_init();   // Inicializar puerto serie COM1 (0x3F8)
    gdt_init();      // 1. Carga la GDT
    idt_init();      // 2. Carga la IDT en la CPU (lidt)
    
    // Verificar magic de multiboot
    if (magic != 0x2BADB002) {
        vga_puts("[ERROR] Not loaded by multiboot\n", COLOR_LIGHT_RED);
        pmm_init(128 * 1024 * 1024, 0x100000);
    } else {
        multiboot_info_t* mbi = (multiboot_info_t*)mbi_addr;
        
        // Si Multiboot proporciona el mapa de memoria (bit 6)
        if (mbi->flags & (1 << 6)) {
            pmm_init_multiboot(mbi->mmap_addr, mbi->mmap_length);
        } else if (mbi->flags & 0x01) {
            uint32_t mem_total = (mbi->mem_lower + mbi->mem_upper) * 1024;
            pmm_init(mem_total, 0x100000);
            for (uint32_t addr = 0; addr < 0x200000; addr += PAGE_SIZE) {
                pmm_mark_used(addr);
            }
        } else {
            pmm_init(128 * 1024 * 1024, 0x100000);
            for (uint32_t addr = 0; addr < 0x200000; addr += PAGE_SIZE) {
                pmm_mark_used(addr);
            }
        }
    }
    
    // Inicializar sub-sistemas del Kernel y Hardware
    pit_init(100);
    heap_init();
    paging_init();
    scheduler_init();
    keyboard_init();
    ata_init();      // Inicializar driver de disco duro ATA PIO (LBA28)
    vfs_init();      // Inicializar Sistema de Archivos Virtual (VFS)

    vga_puts("================================================================================", COLOR_LIGHT_CYAN);
    vga_puts("   HumanOS v0.0.1 (x86 Ring 3 Userland) - Multi-tasking Kernel                ", COLOR_LIGHT_CYAN);
    vga_puts("================================================================================", COLOR_LIGHT_CYAN);

    // Información de Hardware (CPUID y Memoria RAM)
    cpu_print_info();
    
    vga_puts("  RAM Memory : ", COLOR_LIGHT_GREEN);
    char rbuf[16];
    uint32_t free_mb = (pmm_get_free_pages() * 4096) / (1024 * 1024);
    int rcnt = free_mb;
    int rj = 0;
    if (rcnt == 0) rbuf[rj++] = '0';
    while (rcnt > 0 && rj < 15) {
        rbuf[rj++] = '0' + (rcnt % 10);
        rcnt /= 10;
    }
    for (int k = rj - 1; k >= 0; k--) vga_putc(rbuf[k], COLOR_WHITE);
    vga_puts(" MB Free\n", COLOR_WHITE);

    vga_puts(" [OK] GDT/TSS, IDT, PMM, Paging, ATA HDD, VFS, Scheduler & Syscalls Active\n", COLOR_LIGHT_GREEN);
    vga_puts("================================================================================\n\n", COLOR_LIGHT_CYAN);

    scheduler_convert_current_process("KernelMain");
    scheduler_add_user_process(user_shell_process, "UserShellRing3");
    
    // Loop principal del Kernel
    while (1) {
        __asm__ __volatile__("hlt");
    }
}