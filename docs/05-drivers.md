# Hardware Drivers in HumanOS

## Overview

HumanOS includes drivers for several essential hardware devices:
- **PIT (Programmable Interval Timer)**: Timer for scheduling
- **PS/2 Keyboard**: User input
- **VGA**: Text output
- **Serial COM1**: Debug logs
- **ATA PIO**: Hard disk

## 1. PIT (Programmable Interval Timer)

### Description

The PIT 8254/8253 is a timing chip that generates periodic interrupts. HumanOS uses channel 0 to generate IRQ0 at 100 Hz (10 ms per tick).

### PIT Registers

```
I/O Port    Register
────────────────────────────────
0x40        Channel 0 Data Port
0x41        Channel 1 Data Port
0x42        Channel 2 Data Port
0x43        Command/Mode Register
```

### Initialization

#### pit_init()

```c
void pit_init(uint32_t frequency);
```

Process:
1. Calculate divisor: `1193182 / frequency`
2. Send command to PIT (port 0x43: 0x36)
3. Send divisor low byte (port 0x40)
4. Send divisor high byte (port 0x40)

### IRQ0 Handler

#### pit_handler()

```c
void pit_handler(void) {
    scheduler_tick();
}
```

## 2. PS/2 Keyboard

### Description

The PS/2 keyboard uses the 8042 controller to send scancodes to the CPU. HumanOS implements a basic driver that converts scancodes to ASCII characters.

### 8042 Controller Registers

```
I/O Port    Register                R/W
────────────────────────────────────────
0x60        Data Port               R/W
0x64        Command/Status Port     R (status), W (command)
```

### keyboard_getchar()

```c
char keyboard_getchar(void);
```

Returns ASCII character or 0 if no data.

## 3. VGA Text Mode

### Description

VGA in text mode provides an 80x25 character framebuffer with 16 colors.

### Video Memory

```
Address: 0xB8000
Size: 80 * 25 * 2 = 4000 bytes
```

Each character occupies 2 bytes:
- Byte 0: ASCII character
- Byte 1: Color attribute

### VGA Functions

#### vga_puts()

```c
void vga_puts(const char* str, uint8_t color);
```

#### vga_putc()

```c
void vga_putc(char c, uint8_t color);
```

#### vga_clear()

```c
void vga_clear(void);
```

## 4. Serial COM1

### Description

Serial port COM1 (0x3F8) is used for debug logging.

### UART 16550 Registers

```
I/O Port    Register
────────────────────────────────
0x3F8       THR / RBR
0x3F9       IER
0x3FB       LCR
0x3FD       LSR
```

### serial_init()

```c
void serial_init(void);
```

Configures 115200 baud, 8N1.

### serial_write()

```c
void serial_write(const char* str);
```

## 5. ATA PIO (Parallel I/O)

### Description

The ATA PIO driver allows access to IDE/PATA hard disks using LBA28.

### Primary ATA Registers

```
I/O Port    Register                R/W
────────────────────────────────────────
0x1F0       Data Register           R/W
0x1F2       Sector Count            R/W
0x1F3       LBA Low                 R/W
0x1F4       LBA Mid                 R/W
0x1F5       LBA High                R/W
0x1F6       Drive/Head              R/W
0x1F7       Command/Status          R (status), W (command)
```

### ata_init()

```c
void ata_init(void);
```

Detects disk and prints model and size.

### ata_read_sector()

```c
int ata_read_sector(uint32_t lba, uint8_t* buffer);
```

Reads one sector from disk.

### ata_write_sector()

```c
int ata_write_sector(uint32_t lba, const uint8_t* buffer);
```

Writes one sector to disk.

## 6. PIC 8259

### Remapping

The PIC is remapped to IRQs 32-47 to avoid conflicts with CPU exceptions.

```
IRQ Original    IRQ Remapped    Device
─────────────────────────────────────────────
IRQ0            32              PIT Timer
IRQ1            33              Keyboard
IRQ14           46              Primary ATA
```
