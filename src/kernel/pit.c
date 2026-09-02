#include "pit.h"
#include "vga.h"
#include "scheduler.h"
#include <stdint.h>

// Contador de ticks global
static volatile uint64_t pit_ticks = 0;

// Función para escribir en puerto I/O
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

// Inicializar PIT
void pit_init(uint32_t frequency) {
    // Calcular divisor para la frecuencia deseada
    uint32_t divisor = PIT_FREQUENCY / frequency;
    
    // Enviar comando al PIT: Channel 0, Access word, Mode 3 (square wave), Binary
    outb(PIT_PORT_COMMAND, PIT_CMD_CHANNEL0 | PIT_CMD_ACCESS_WORD | PIT_CMD_MODE_3 | PIT_CMD_BINARY);
    
    // Enviar divisor (low byte primero, luego high byte)
    outb(PIT_PORT_CHANNEL0, divisor & 0xFF);
    outb(PIT_PORT_CHANNEL0, (divisor >> 8) & 0xFF);
    
    vga_puts("[PIT] Initialized at ", 0x1A);
    
    // Imprimir frecuencia
    char buf[16];
    int count = frequency;
    int j = 0;
    while (count > 0 && j < 15) {
        buf[j++] = '0' + (count % 10);
        count /= 10;
    }
    for (int k = j - 1; k >= 0; k--) {
        vga_putc(buf[k], 0x1A);
    }
    vga_puts(" Hz\n", 0x1A);
}

// Handler de interrupción del PIT (IRQ0)
void pit_handler(void) {
    pit_ticks++;
    
    // Llamar al scheduler_tick para marcar schedule_needed
    scheduler_tick();
    
    // Enviar EOI al Master PIC (IRQ0 está en el Master)
    outb(0x20, 0x20);
}

// Obtener número de ticks
uint64_t pit_get_ticks(void) {
    return pit_ticks;
}

// Retardo en milisegundos (aproximado)
void pit_sleep(uint32_t milliseconds) {
    uint64_t start_ticks = pit_ticks;
    uint64_t target_ticks = start_ticks + (milliseconds * 100) / 1000;  // Asumiendo 100 Hz
    
    while (pit_ticks < target_ticks) {
        __asm__ __volatile__("hlt");  // Halt hasta la próxima interrupción
    }
}
