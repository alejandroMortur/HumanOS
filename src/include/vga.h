#ifndef VGA_H
#define VGA_H

#include <stdint.h>

// Paleta de Colores VGA (Fondo Negro 0x0_)
#define COLOR_BLACK         0x00
#define COLOR_BLUE          0x01
#define COLOR_GREEN         0x02
#define COLOR_CYAN          0x03
#define COLOR_RED           0x04
#define COLOR_MAGENTA       0x05
#define COLOR_BROWN         0x06
#define COLOR_LIGHT_GREY    0x07
#define COLOR_DARK_GREY     0x08
#define COLOR_LIGHT_BLUE    0x09
#define COLOR_LIGHT_GREEN   0x0A
#define COLOR_LIGHT_CYAN    0x0B
#define COLOR_LIGHT_RED     0x0C
#define COLOR_LIGHT_MAGENTA 0x0D
#define COLOR_YELLOW        0x0E
#define COLOR_WHITE         0x0F

void vga_init(void);
void vga_clear(void);
void vga_putc(char c, uint8_t color);
void vga_puts(const char* str, uint8_t color);

#endif