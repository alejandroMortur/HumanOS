#ifndef PIT_H
#define PIT_H

#include <stdint.h>

// Puertos del PIT
#define PIT_PORT_COMMAND 0x43
#define PIT_PORT_CHANNEL0 0x40
#define PIT_PORT_CHANNEL1 0x41
#define PIT_PORT_CHANNEL2 0x42

// Comandos del PIT
#define PIT_CMD_CHANNEL0 0x00
#define PIT_CMD_CHANNEL1 0x40
#define PIT_CMD_CHANNEL2 0x80
#define PIT_CMD_READBACK 0xC0

#define PIT_CMD_ACCESS_LATCH 0x00
#define PIT_CMD_ACCESS_LOW 0x10
#define PIT_CMD_ACCESS_HIGH 0x20
#define PIT_CMD_ACCESS_WORD 0x30

#define PIT_CMD_MODE_0 0x00  // Interrupt on terminal count
#define PIT_CMD_MODE_1 0x02  // Hardware retriggerable one-shot
#define PIT_CMD_MODE_2 0x04  // Rate generator
#define PIT_CMD_MODE_3 0x06  // Square wave generator
#define PIT_CMD_MODE_4 0x08  // Software triggered strobe
#define PIT_CMD_MODE_5 0x0A  // Hardware triggered strobe

#define PIT_CMD_BINARY 0x00
#define PIT_CMD_BCD 0x01

// Frecuencia base del PIT: 1.193182 MHz
#define PIT_FREQUENCY 1193182

// Funciones PIT
void pit_init(uint32_t frequency);
void pit_handler(void);
uint64_t pit_get_ticks(void);
void pit_sleep(uint32_t milliseconds);

#endif
