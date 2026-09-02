# Compilador y Banderas
CC = gcc
AS = nasm
CFLAGS = -m32 -c -std=c99 -ffreestanding -O2 -Wall -Wextra -Isrc/include
ASFLAGS = -f elf32

# Rutas de Archivos
LINKER = src/arch/i386/linker.ld
KERNEL_BIN = build/mykernel.bin
ISO_OUT = build/HumanOS.iso

# Mapeo de Objetos
OBJS = build/boot.o \
       build/gdt_flush.o \
       build/interrupts.o \
       build/gdt.o \
       build/idt.o \
       build/pmm.o \
       build/pit.o \
       build/heap.o \
       build/paging.o \
       build/scheduler.o \
       build/keyboard.o \
       build/vga.o  \
       build/main.o

all: $(ISO_OUT)

# Reglas Ensamblador
build/boot.o: src/arch/i386/boot.s
	$(AS) $(ASFLAGS) $< -o $@

build/gdt_flush.o: src/arch/i386/gdt_flush.s
	$(AS) $(ASFLAGS) $< -o $@

build/interrupts.o: src/arch/i386/interrupts.s
	$(AS) $(ASFLAGS) $< -o $@

# Reglas C
build/gdt.o: src/kernel/gdt.c
	$(CC) $(CFLAGS) $< -o $@

build/idt.o: src/kernel/idt.c
	$(CC) $(CFLAGS) $< -o $@

build/pmm.o: src/kernel/pmm.c
	$(CC) $(CFLAGS) $< -o $@

build/pit.o: src/kernel/pit.c
	$(CC) $(CFLAGS) $< -o $@

build/heap.o: src/kernel/heap.c
	$(CC) $(CFLAGS) $< -o $@

build/paging.o: src/kernel/paging.c
	$(CC) $(CFLAGS) $< -o $@

build/scheduler.o: src/kernel/scheduler.c
	$(CC) $(CFLAGS) $< -o $@

build/keyboard.o: src/drivers/keyboard.c
	$(CC) $(CFLAGS) $< -o $@

build/vga.o: src/drivers/vga.c
	$(CC) $(CFLAGS) $< -o $@

build/main.o: src/kernel/main.c
	$(CC) $(CFLAGS) $< -o $@

# Enlazado y ISO
$(KERNEL_BIN): $(OBJS)
	$(CC) -m32 -T $(LINKER) -o $@ -nostdlib $(OBJS) -lgcc

$(ISO_OUT): $(KERNEL_BIN)
	cp $(KERNEL_BIN) iso/boot/mykernel.bin
	echo 'set timeout=0' > iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	echo 'menuentry "HumanOS" { multiboot /boot/mykernel.bin; boot; }' >> iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_OUT) iso

clean:
	rm -rf build/* iso/boot/mykernel.bin $(ISO_OUT)