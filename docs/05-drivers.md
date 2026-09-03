# Drivers de Hardware en HumanOS

## Visión General

HumanOS incluye drivers para varios dispositivos de hardware esenciales:
- **PIT (Programmable Interval Timer)**: Timer para scheduling
- **Teclado PS/2**: Entrada de usuario
- **VGA**: Salida de texto
- **Serial COM1**: Logs de depuración
- **ATA PIO**: Disco duro

## 1. PIT (Programmable Interval Timer)

### Descripción

El PIT 8254/8253 es un chip de temporización que genera interrupciones periódicas. HumanOS usa el canal 0 para generar IRQ0 a 100 Hz (10 ms por tick).

### Registros del PIT

```
I/O Port    Registro
────────────────────────────────
0x40        Channel 0 Data Port
0x41        Channel 1 Data Port
0x42        Channel 2 Data Port
0x43        Command/Mode Register
```

### Inicialización

#### pit_init()

```c
void pit_init(uint32_t frequency);
```

Proceso:
1. Calcula divisor: `1193182 / frequency`
2. Envía comando al PIT (puerto 0x43: 0x36)
3. Envía divisor low byte (puerto 0x40)
4. Envía divisor high byte (puerto 0x40)

### Handler de IRQ0

#### pit_handler()

```c
void pit_handler(void) {
    scheduler_tick();
}
```

## 2. Teclado PS/2

### Descripción

El teclado PS/2 usa el controlador 8042 para enviar scancodes al CPU. HumanOS implementa un driver básico que convierte scancodes a caracteres ASCII.

### Registros del Controlador 8042

```
I/O Port    Registro                R/W
────────────────────────────────────────
0x60        Data Port               R/W
0x64        Command/Status Port     R (status), W (command)
```

### keyboard_getchar()

```c
char keyboard_getchar(void);
```

Retorna carácter ASCII o 0 si no hay dato.

## 3. VGA Text Mode

### Descripción

La VGA en modo texto proporciona un framebuffer de 80x25 caracteres con 16 colores.

### Memoria de Video

```
Dirección: 0xB8000
Tamaño: 80 * 25 * 2 = 4000 bytes
```

Cada carácter ocupa 2 bytes:
- Byte 0: Carácter ASCII
- Byte 1: Atributo de color

### Funciones VGA

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

### Descripción

El puerto serie COM1 (0x3F8) se usa para logs de depuración.

### Registros del UART 16550

```
I/O Port    Registro
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

Configura 115200 baud, 8N1.

### serial_write()

```c
void serial_write(const char* str);
```

## 5. ATA PIO (Parallel I/O)

### Descripción

El driver ATA PIO permite acceso a discos duros IDE/PATA usando LBA28.

### Registros ATA Primario

```
I/O Port    Registro                R/W
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

Detecta disco e imprime modelo y tamaño.

### ata_read_sector()

```c
int ata_read_sector(uint32_t lba, uint8_t* buffer);
```

Lee un sector del disco.

### ata_write_sector()

```c
int ata_write_sector(uint32_t lba, const uint8_t* buffer);
```

Escribe un sector al disco.

## 6. PIC 8259

### Re-mapeo

El PIC se re-mapea a IRQs 32-47 para evitar conflictos con excepciones del CPU.

```
IRQ Original    IRQ Re-mapeado    Dispositivo
─────────────────────────────────────────────
IRQ0            32                PIT Timer
IRQ1            33                Keyboard
IRQ14           46                Primary ATA
```
