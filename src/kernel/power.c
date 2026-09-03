#include "power.h"
#include "vga.h"
#include "serial.h"
#include "vfs.h"
#include "ata.h"
#include "scheduler.h"
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("out %%al, %%dx" : : "a"(val), "d"(port));
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ __volatile__("out %%ax, %%dx" : : "a"(val), "d"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("in %%dx, %%al" : "=a"(ret) : "d"(port));
    return ret;
}

// Reiniciar el sistema de forma ordenada
void power_reboot(void) {
    vga_puts("\n[POWER] Initiating orderly system reboot...\n", COLOR_LIGHT_RED);
    serial_puts("\n[POWER] Initiating orderly system reboot...\n");

    // 1. Sincronizar el sistema de archivos (VFS) y limpiar caché del disco ATA
    vga_puts(" [1/3] Syncing file system & flushing disk cache... ", COLOR_LIGHT_CYAN);
    serial_puts(" [1/3] Syncing file system & flushing disk cache... ");
    vfs_sync();
    ata_flush();
    vga_puts("[OK]\n", COLOR_LIGHT_GREEN);
    serial_puts("[OK]\n");

    // 2. Finalizar todos los procesos del scheduler
    vga_puts(" [2/3] Terminating user & background processes...   ", COLOR_LIGHT_CYAN);
    serial_puts(" [2/3] Terminating user & background processes...   ");
    scheduler_terminate_all();
    vga_puts("[OK]\n", COLOR_LIGHT_GREEN);
    serial_puts("[OK]\n");

    // 3. Reiniciar hardware vía controlador 8042 PS/2
    vga_puts(" [3/3] Sending reset signal to PS/2 controller...   ", COLOR_LIGHT_CYAN);
    serial_puts(" [3/3] Sending reset signal to PS/2 controller...   ");
    __asm__ __volatile__("cli");
    vga_puts("[OK]\n", COLOR_LIGHT_GREEN);
    serial_puts("[OK]\n");

    uint8_t temp;
    do {
        temp = inb(0x64);
        if (temp & 1) (void)inb(0x60);
    } while (temp & 2);

    // Pulso de reset al controlador PS/2
    outb(0x64, 0xFE);

    // Fallback: Forzar Triple Fault si falla el reset del 8042
    static struct { uint16_t limit; uint32_t base; } __attribute__((packed)) null_idt = {0, 0};
    __asm__ __volatile__("lidt %0; int3" : : "m"(null_idt));
    while (1) { __asm__ __volatile__("hlt"); }
}

// Apagar el sistema de forma ordenada (vía ACPI / QEMU / Bochs / VirtualBox ports)
void power_shutdown(void) {
    vga_puts("\n[POWER] Initiating orderly system shutdown...\n", COLOR_LIGHT_RED);
    serial_puts("\n[POWER] Initiating orderly system shutdown...\n");

    // 1. Sincronizar el sistema de archivos (VFS) y limpiar caché del disco ATA
    vga_puts(" [1/3] Syncing file system & flushing disk cache... ", COLOR_LIGHT_CYAN);
    serial_puts(" [1/3] Syncing file system & flushing disk cache... ");
    vfs_sync();
    ata_flush();
    vga_puts("[OK]\n", COLOR_LIGHT_GREEN);
    serial_puts("[OK]\n");

    // 2. Finalizar todos los procesos del scheduler
    vga_puts(" [2/3] Terminating user & background processes...   ", COLOR_LIGHT_CYAN);
    serial_puts(" [2/3] Terminating user & background processes...   ");
    scheduler_terminate_all();
    vga_puts("[OK]\n", COLOR_LIGHT_GREEN);
    serial_puts("[OK]\n");

    // 3. Desactivar interrupciones y multitarea de CPU
    vga_puts(" [3/3] Disabling interrupts and CPU multitasking... ", COLOR_LIGHT_CYAN);
    serial_puts(" [3/3] Disabling interrupts and CPU multitasking... ");
    __asm__ __volatile__("cli");
    vga_puts("[OK]\n", COLOR_LIGHT_GREEN);
    serial_puts("[OK]\n");

    vga_puts("\n[POWER] System halted cleanly. Powering off...\n", COLOR_LIGHT_GREEN);
    serial_puts("\n[POWER] System halted cleanly. Powering off...\n");

    // Enviar señal de apagado ACPI a los hipervisores (QEMU, Bochs, VirtualBox)
    outw(0x604, 0x2000);
    outw(0x604, 0x3400);

    outw(0xB004, 0x2000);

    outw(0x4004, 0x3400);
    outw(0x4004, 0x2000);
    outw(0x600, 0x3400);
    outw(0x600, 0x2000);

    outw(0x5001, 0x0000);

    // Detención total de la CPU
    while (1) {
        __asm__ __volatile__("hlt");
    }
}
