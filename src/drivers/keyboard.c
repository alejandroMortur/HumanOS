#include "vga.h"
#include "idt.h"
#include <stdint.h>

extern void irq0_stub(void);
extern void irq1_stub(void);

// Corregido: Sintaxis exacta GCC para inb / outb en x86
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("in %%dx, %%al" : "=a"(ret) : "d"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("out %%al, %%dx" : : "a"(val), "d"(port));
}

static const char scancode_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
     0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
     0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
   '*',   0, ' '
};

void pic_remap(void) {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20); // Master PIC a int 32
    outb(0xA1, 0x28); // Slave PIC a int 40
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    
    // Desenmascara IRQ0 (PIT - bit 0 a 0) e IRQ1 (Teclado - bit 1 a 0)
    outb(0x21, 0xFC);  // 0xFC = 11111100 (IRQ0 y IRQ1 habilitados)
    outb(0xA1, 0xFF);
}

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60); // Lee el scancode real del chip

    if (!(scancode & 0x80)) { // Tecla presionada (Key Press)
        if (scancode < sizeof(scancode_ascii)) {
            char c = scancode_ascii[scancode];
            if (c != 0) {
                vga_putc(c, 0x1E);
            }
        }
    }

    outb(0x20, 0x20); // EOI (End of Interrupt) al Master PIC
}

void keyboard_init(void) {
    pic_remap();
    idt_set_gate(32, (uint32_t)irq0_stub, 0x08, 0x8E);  // IRQ0 - PIT
    idt_set_gate(33, (uint32_t)irq1_stub, 0x08, 0x8E);  // IRQ1 - Keyboard

    // Vaciar el buffer del controlador PS/2 si tenia datos colgados del boot
    while (inb(0x64) & 0x01) {
        inb(0x60);
    }

    __asm__ __volatile__("sti"); // Activa las interrupciones en la CPU
}