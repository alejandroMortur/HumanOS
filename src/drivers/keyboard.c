#include "vga.h"
#include "idt.h"
#include <stdint.h>

extern void irq0_stub(void);
extern void irq1_stub(void);

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("in %%dx, %%al" : "=a"(ret) : "d"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("out %%al, %%dx" : : "a"(val), "d"(port));
}

#define KEY_UP    ((char)0x80)
#define KEY_DOWN  ((char)0x81)

static const char scancode_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
     0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
     0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
   '*',   0, ' ',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0, KEY_UP, 0, 0, 0, 0, 0, 0, 0,
     0, KEY_DOWN
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
    
    outb(0x21, 0xFC);  // IRQ0 y IRQ1 habilitados
    outb(0xA1, 0xFF);
}

static char kbd_buffer[128];
static volatile uint32_t kbd_head = 0;
static volatile uint32_t kbd_tail = 0;
static uint8_t is_e0_prefix = 0;

char keyboard_getchar(void) {
    if (kbd_head == kbd_tail) {
        return 0;
    }
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % 128;
    return c;
}

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);

    if (scancode == 0xE0) {
        is_e0_prefix = 1;
        outb(0x20, 0x20);
        return;
    }

    if (!(scancode & 0x80)) { // Key Press
        char c = 0;
        if (is_e0_prefix) {
            if (scancode == 0x48) c = KEY_UP;
            else if (scancode == 0x50) c = KEY_DOWN;
            is_e0_prefix = 0;
        } else if (scancode < sizeof(scancode_ascii)) {
            c = scancode_ascii[scancode];
        }

        if (c != 0) {
            uint32_t next_head = (kbd_head + 1) % 128;
            if (next_head != kbd_tail) {
                kbd_buffer[kbd_head] = c;
                kbd_head = next_head;
            }
        }
    } else {
        is_e0_prefix = 0;
    }

    outb(0x20, 0x20);
}

void keyboard_init(void) {
    pic_remap();
    idt_set_gate(32, (uint32_t)irq0_stub, 0x08, 0x8E);  // IRQ0 - PIT
    idt_set_gate(33, (uint32_t)irq1_stub, 0x08, 0x8E);  // IRQ1 - Keyboard

    while (inb(0x64) & 0x01) {
        inb(0x60);
    }

    __asm__ __volatile__("sti");
}