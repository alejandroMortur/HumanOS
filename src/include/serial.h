#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

#define COM1_PORT 0x3F8

// Funciones del driver de puerto serie UART 16550
void serial_init(void);
int serial_received(void);
char serial_read(void);
int is_transmit_empty(void);
void serial_putc(char c);
void serial_puts(const char* str);

#endif
