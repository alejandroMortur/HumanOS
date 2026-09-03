# HumanOS Documentation

This folder contains the complete technical documentation for HumanOS v0.0.1, a 32-bit operating system for x86 (i686) architecture.

## Documentation Index

### [01-architecture.md](01-architecture.md)
System architecture overview, main components, boot flow, directory structure, and configuration constants.

### [02-memory-management.md](02-memory-management.md)
Complete memory management documentation:
- PMM (Physical Memory Manager) - Bitmap allocator
- VMM (Virtual Memory Manager) - Identity-mapped paging
- Heap - Dynamic memory allocator

### [03-scheduler.md](03-scheduler.md)
Scheduler and process management documentation:
- Preemptive multitasking (PIT Timer)
- Context switch in assembly
- Ring 0 and Ring 3 processes
- Process states
- TSS and stacks

### [04-syscalls.md](04-syscalls.md)
Syscalls and Ring 3 transition documentation:
- int 0x80 with DPL=3
- Syscall stub in assembly
- Implemented syscalls (yield, exit, write, read, malloc, getpid)
- User wrappers
- Stack frames and calling convention

### [05-drivers.md](05-drivers.md)
Hardware drivers documentation:
- PIT (Programmable Interval Timer)
- PS/2 Keyboard
- VGA Text Mode
- Serial COM1
- ATA PIO (hard disk)
- PIC 8259

### [06-filesystem.md](06-filesystem.md)
VFS file system documentation:
- Data structures
- Disk layout
- Operations (create, write, read, list)
- ATA disk synchronization

### [07-userland.md](07-userland.md)
User shell documentation:
- user_shell_process in Ring 3
- Implemented commands
- Command history
- Autocomplete
- Used syscalls

## Main Features

### Bootstrapping
- ✅ Multiboot1/2 Compliant Header
- ✅ GDT with Kernel/User segments and TSS
- ✅ IDT with 32 exception ISRs
- ✅ PIC 8259 remapping

### Memory Management
- ✅ PMM (Physical Memory Manager) - Bitmap allocator
- ✅ VMM (Virtual Memory Manager) - Active paging
- ✅ kmalloc/kfree - Dynamic memory allocator

### Processes and Tasks
- ✅ TCB (Task Control Block) structured
- ✅ Process creation with fake stack frame
- ✅ Robust context switch in assembly
- ✅ Cooperative multitasking scheduler
- ✅ Preemptive multitasking scheduler (PIT Timer)
- ✅ Ring 0 (kernel) and Ring 3 (user) processes

### Hardware Drivers
- ✅ PIT/APIC Timer - 100 Hz
- ✅ PS/2 Keyboard - Basic driver
- ✅ VGA Text Framebuffer - Text output
- ✅ Serial Port COM1 - Debug logs

### Syscalls and IPC
- ✅ int 0x80 with DPL=3 (accessible from Ring 3)
- ✅ Syscalls: yield, exit, write, read, malloc, getpid

### File System
- ✅ VFS (Virtual File System)
- ✅ ATA PIO integration
- ✅ Operations: create, write, read, list

### Userland
- ✅ Interactive shell in Ring 3
- ✅ Commands: help, ls, cat, touch, write, diskinfo, sysinfo, whoami, hostname, uname, clear, ps, reboot, shutdown, exit
- ✅ Command history (10 commands)
- ✅ Tab autocomplete

## Tech Stack

- **Architecture**: x86 (i686) 32-bit
- **Mode**: Protected with paging
- **Rings**: Ring 0 (kernel) and Ring 3 (user)
- **Assembly**: NASM (interrupts.s, boot.s, gdt_flush.s)
- **C**: GCC (kernel, drivers)
- **Linker**: LD with custom script
- **Bootloader**: GRUB/Limine (Multiboot)

## Important Constants

- **PAGE_SIZE**: 4096 bytes (4 KB)
- **KERNEL_STACK_SIZE**: 8192 bytes (8 KB)
- **MAX_PROCESSES**: 32 concurrent processes
- **MAX_FILES**: 64 files in VFS
- **MAX_FILE_SIZE**: 1024 bytes (1 KB)
- **PIT Frequency**: 100 Hz (10 ms per tick)
- **Scheduler Quantum**: 10 ticks = 100 ms

## Building

```bash
make clean
make
```

This generates:
- `build/kernel.bin` - Kernel binary
- `build/HumanOS.iso` - Bootable ISO image
