#include "power.h"
#include "vga.h"
#include "serial.h"
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

// Reiniciar el sistema a través del controlador de teclado PS/2 (0x64 -> 0xFE)
void sys_reboot(void) {
    vga_puts("\n[POWER] Rebooting HumanOS System...\n", COLOR_LIGHT_RED);
    serial_puts("\n[POWER] Rebooting HumanOS System...\n");
    
    uint8_t temp;
    do {
        temp = inb(0x64);
        if (temp & 1) (void)inb(0x60);
    } while (temp & 2);
    
    // Pulso de reset al controlador PS/2
    outb(0x64, 0xFE);
    
    // Fallback: Forzar Triple Fault si falla el reset del 8042
    static struct { uint16_t limit; uint32_t base; } __attribute__((packed)) null_idt = {0, 0};
    __asm__ __volatile__("cli; lidt %0; int3" : : "m"(null_idt));
    while (1) { __asm__ __volatile__("hlt"); }
}

// Apagar el sistema (vía ACPI / QEMU / Bochs / VirtualBox ports)
void sys_shutdown(void) {
    vga_puts("\n[POWER] Shutting down HumanOS...\n", COLOR_LIGHT_RED);
    serial_puts("\n[POWER] Shutting down HumanOS...\n");
    
    // QEMU shutdown port
    outw(0x604, 0x2000);
    
    // Bochs / QEMU old shutdown port
    outw(0xB004, 0x2000);
    
    // VirtualBox shutdown port
    outw(0x4004, 0x3400);
    
    __asm__ __volatile__("cli");
    while (1) {
        __asm__ __volatile__("hlt");
    }
}
