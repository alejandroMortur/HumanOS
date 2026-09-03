#include "vga.h"

static volatile uint16_t* const VGA_BUFFER = (uint16_t*) 0xB8000;
static const int VGA_WIDTH = 80;
static const int VGA_HEIGHT = 25;

static int cursor_x = 0;
static int cursor_y = 0;

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

// Scroll de pantalla cuando se llega al final de la linea 25
static void vga_scroll(void) {
    uint8_t color = COLOR_WHITE;
    if (cursor_y >= VGA_HEIGHT) {
        for (int i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
            VGA_BUFFER[i] = VGA_BUFFER[i + VGA_WIDTH];
        }
        for (int i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
            VGA_BUFFER[i] = vga_entry(' ', color);
        }
        cursor_y = VGA_HEIGHT - 1;
    }
}

void vga_init(void) {
    uint8_t color = COLOR_WHITE; // Blanco sobre Negro
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VGA_BUFFER[i] = vga_entry(' ', color);
    }
    cursor_x = 0;
    cursor_y = 0;
}

void vga_clear(void) {
    vga_init();
}

void vga_putc(char c, uint8_t color) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') { // Soporte para retroceso (Backspace)
        if (cursor_x > 0) {
            cursor_x--;
            const int index = cursor_y * VGA_WIDTH + cursor_x;
            VGA_BUFFER[index] = vga_entry(' ', color);
        }
    } else {
        const int index = cursor_y * VGA_WIDTH + cursor_x;
        VGA_BUFFER[index] = vga_entry((unsigned char)c, color);
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    vga_scroll(); // Comprueba si hace falta hacer scroll hacia abajo
}

void vga_puts(const char* str, uint8_t color) {
    for (int i = 0; str[i] != '\0'; i++) {
        vga_putc(str[i], color);
    }
}