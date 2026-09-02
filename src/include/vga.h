#ifndef VGA_H
#define VGA_H

#include <stdint.h>

void vga_init(void);
void vga_putc(char c, uint8_t color);
void vga_puts(const char* str, uint8_t color);

#endif