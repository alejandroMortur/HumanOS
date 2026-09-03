# Compilador y Banderas
CC = gcc
AS = nasm
CFLAGS = -m32 -c -std=c99 -ffreestanding -O2 -Wall -Wextra -Isrc/include
ASFLAGS = -f elf32

# Rutas de Archivos
LINKER = src/arch/i386/linker.ld
KERNEL_BIN = build/mykernel.bin
ISO_OUT = build/HumanOS.iso
DISK_IMG = build/hdd.img

# Mapeo de Objetos
OBJS = build/boot.o \
       build/gdt_flush.o \
       build/interrupts.o \
       build/gdt.o \
       build/idt.o \
       build/isr.o \
       build/syscall.o \
       build/spinlock.o \
       build/semaphore.o \
       build/serial.o \
       build/power.o \
       build/cpu.o \
       build/ata.o \
       build/vfs.o \
       build/pmm.o \
       build/pit.o \
       build/heap.o \
       build/paging.o \
       build/scheduler.o \
       build/keyboard.o \
       build/vga.o  \
       build/libc.o \
       build/elf.o \
       build/main.o

.PHONY: all clean disk run

all: $(ISO_OUT) $(DISK_IMG)

# Crear disco virtual persistente (10 MB)
$(DISK_IMG):
	dd if=/dev/zero of=$(DISK_IMG) bs=1M count=10 2>/dev/null || fsutil file createNew $(DISK_IMG) 10485760

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

build/isr.o: src/kernel/isr.c
	$(CC) $(CFLAGS) $< -o $@

build/syscall.o: src/kernel/syscall.c
	$(CC) $(CFLAGS) $< -o $@

build/spinlock.o: src/kernel/spinlock.c
	$(CC) $(CFLAGS) $< -o $@

build/semaphore.o: src/kernel/semaphore.c
	$(CC) $(CFLAGS) $< -o $@

build/serial.o: src/drivers/serial.c
	$(CC) $(CFLAGS) $< -o $@

build/power.o: src/kernel/power.c
	$(CC) $(CFLAGS) $< -o $@

build/cpu.o: src/kernel/cpu.c
	$(CC) $(CFLAGS) $< -o $@

build/ata.o: src/drivers/ata.c
	$(CC) $(CFLAGS) $< -o $@

build/vfs.o: src/kernel/vfs.c
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

build/libc.o: src/user/libc.c
	$(CC) $(CFLAGS) $< -o $@

build/elf.o: src/kernel/elf.c
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

# Ejecutar en QEMU con disco persistente
run-qemu: $(ISO_OUT) $(DISK_IMG)
	qemu-system-i386 -cdrom $(ISO_OUT) -drive id=disk,file=$(DISK_IMG),if=ide,media=disk,format=raw -device ide-hd,drive=disk,bus=ide.0,unit=0

# Ejecutar en QEMU con disco persistente (alias)
run: run-qemu