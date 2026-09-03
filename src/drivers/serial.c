#include "serial.h"

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("in %%dx, %%al" : "=a"(ret) : "d"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("out %%al, %%dx" : : "a"(val), "d"(port));
}

// Inicializar el puerto serie COM1 (UART 16550) a 38400 baudios
void serial_init(void) {
    outb(COM1_PORT + 1, 0x00);    // Desactivar interrupciones
    outb(COM1_PORT + 3, 0x80);    // Habilitar DLAB (divisor de baudios)
    outb(COM1_PORT + 0, 0x03);    // Divisor 3 -> 38400 baudios (low byte)
    outb(COM1_PORT + 1, 0x00);    // (high byte)
    outb(COM1_PORT + 3, 0x03);    // 8 bits, sin paridad, 1 bit de parada
    outb(COM1_PORT + 2, 0xC7);    // Habilitar FIFO, limpiar buffers
    outb(COM1_PORT + 4, 0x0B);    // Habilitar IRQs y lineas RTS/DSR
}

int serial_received(void) {
    return inb(COM1_PORT + 5) & 1;
}

char serial_read(void) {
    while (serial_received() == 0);
    return inb(COM1_PORT);
}

int is_transmit_empty(void) {
    return inb(COM1_PORT + 5) & 0x20;
}

void serial_putc(char c) {
    while (is_transmit_empty() == 0);
    outb(COM1_PORT, c);
}

void serial_puts(const char* str) {
    if (str == ((void*)0)) return;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            serial_putc('\r');
        }
        serial_putc(str[i]);
    }
}
