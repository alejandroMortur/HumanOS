# HumanOS Architecture

## Overview

HumanOS is a 32-bit operating system for x86 (i686) architecture designed to run in protected mode with preemptive multitasking, virtual memory management, and Ring 3 user mode transition.

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     User Space (Ring 3)                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐        │
│  │ User Shell   │  │ ELF Binaries │  │ User Apps    │        │
│  └──────────────┘  └──────────────┘  └──────────────┘        │
└─────────────────────────────┬───────────────────────────────┘
                              │ int 0x80 (Syscalls)
┌─────────────────────────────┴───────────────────────────────┐
│                    Kernel Space (Ring 0)                     │
│  ┌────────────────────────────────────────────────────────┐ │
│  │  Scheduler & Process Management                         │ │
│  │  - Preemptive Multitasking (PIT Timer)                │ │
│  │  - Context Switch (scheduler_yield_asm)                │ │
│  │  - Ring 0 & Ring 3 Processes                           │ │
│  └────────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────┐ │
│  │  Memory Management                                     │ │
│  │  - PMM (Physical Memory Manager) - Bitmap               │ │
│  │  - VMM (Virtual Memory Manager) - Paging               │ │
│  │  - Heap (kmalloc/kfree)                                │ │
│  └────────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────┐ │
│  │  File System & Storage                                  │ │
│  │  - VFS (Virtual File System)                           │ │
│  │  - ATA PIO Driver (LBA28)                              │ │
│  │  - ELF Loader                                          │ │
│  └────────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────┐ │
│  │  Hardware Drivers                                      │ │
│  │  - PIT Timer (100 Hz)                                  │ │
│  │  - PS/2 Keyboard                                       │ │
│  │  - VGA Text Framebuffer                                │ │
│  │  - Serial COM1                                         │ │
│  └────────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────┐ │
│  │  Core Kernel Services                                  │ │
│  │  - GDT/TSS (Segmentation)                              │ │
│  │  - IDT/ISR (Interrupts & Exceptions)                   │ │
│  │  - Syscall Handler (int 0x80)                         │ │
│  └────────────────────────────────────────────────────────┘ │
└─────────────────────────────┬───────────────────────────────┘
                              │ Hardware Access
┌─────────────────────────────┴───────────────────────────────┐
│                      Hardware Layer                         │
│  CPU (x86)  |  RAM  |  ATA HDD  |  Keyboard  |  VGA       │
└─────────────────────────────────────────────────────────────┘
```

## Main Components

### 1. Bootstrapping
- **Multiboot Header**: Kernel loading by GRUB/Limine
- **GDT**: Global descriptor table with Kernel/User segments and TSS
- **IDT**: Interrupt descriptor table with 32 exception ISRs
- **PIC 8259**: Interrupt remapping (IRQ0-15)

### 2. Memory Management
- **PMM**: Physical memory manager using bitmap (up to 4 GB)
- **VMM**: Identity-mapped paging for first 4 MB
- **Heap**: Dynamic memory allocator for kernel

### 3. Scheduler & Processes
- **Preemptive Multitasking**: PIT timer at 100 Hz (switch every 100 ms)
- **Context Switch**: Assembly implementation (scheduler_yield_asm)
- **Ring 0 Processes**: Kernel processes with kernel_stack
- **Ring 3 Processes**: User processes with transition via iretd
- **TCB**: Task Control Block with state, context, and metadata

### 4. Syscalls
- **int 0x80**: System calls with DPL=3 (accessible from Ring 3)
- **Available syscalls**: yield, exit, write, read, malloc, getpid

### 5. Hardware Drivers
- **PIT**: Programmable Interval Timer for scheduling
- **PS/2 Keyboard**: Input driver with character map
- **VGA**: Text output with colors (80x25)
- **Serial COM1**: Serial port for debug logs
- **ATA PIO**: Hard disk driver with LBA28

### 6. File System
- **VFS**: Virtual File System simple
- **Storage**: Metadata in sector 100, data from sector 101
- **Operations**: create, write, read, list

### 7. Userland
- **Interactive Shell**: Ring 3 CLI with Bash-style commands
- **Commands**: help, ls, cat, touch, write, diskinfo, sysinfo, whoami, hostname, uname, clear, ps, reboot, shutdown, exit
- **History**: Last 10 commands with navigation (up/down arrows)
- **Autocomplete**: Tab completion for commands and files

### 8. ELF Loader
- **Validation**: Verifies ELF magic and x86 32-bit architecture
- **Loading**: Loads PT_LOAD segments to memory
- **Execution**: Creates Ring 3 process from ELF binary

### 9. CPU Info
- **CPUID**: Gets vendor, model, cores, brand string

### 10. Power Management
- **Reboot**: System reboot via ACPI/keyboard
- **Shutdown**: System power off

## Boot Flow

1. **Bootloader (GRUB/Limine)**: Loads kernel.bin and passes control
2. **kernel_main**: Initializes subsystems in order:
   - vga_init() → Text screen
   - serial_init() → Serial COM1
   - gdt_init() → Segmentation and TSS
   - idt_init() → Interrupts and exceptions
   - pmm_init_multiboot() → Physical memory
   - pit_init(100) → Timer at 100 Hz
   - heap_init() → Dynamic heap
   - paging_init() → Paging
   - scheduler_init() → Scheduler
   - keyboard_init() → Keyboard + PIC + STI
   - ata_init() → ATA hard disk
   - vfs_init() → File system
3. **Convert to Process**: kernel_main → KernelMain (PID 1)
4. **Create User Process**: user_shell_process → UserShellRing3 (PID 2)
5. **Main Loop**: Kernel enters hlt, scheduler handles multitasking

## Directory Structure

```
HumanOS/
├── src/
│   ├── arch/i386/          # x86-specific code
│   │   ├── boot.s          # Multiboot header and entry point
│   │   ├── gdt_flush.s     # Load GDT/TSS into CPU
│   │   ├── interrupts.s    # ISR stubs, IRQ handlers, scheduler_yield_asm
│   │   └── linker.ld       # Linker script
│   ├── kernel/             # Kernel code
│   │   ├── main.c          # kernel_main and user shell
│   │   ├── scheduler.c     # Scheduler and process management
│   │   ├── pmm.c           # Physical Memory Manager
│   │   ├── paging.c        # Virtual Memory Manager
│   │   ├── heap.c          # Dynamic heap
│   │   ├── gdt.c           # Global Descriptor Table
│   │   ├── idt.c           # Interrupt Descriptor Table
│   │   ├── isr.c           # Exception handlers
│   │   ├── syscall.c       # Syscall handler
│   │   ├── pit.c           # PIT Timer driver
│   │   ├── keyboard.c      # PS/2 Keyboard driver
│   │   ├── ata.c           # ATA PIO driver
│   │   ├── vfs.c           # Virtual File System
│   │   ├── elf.c           # ELF loader
│   │   ├── cpu.c           # CPUID info
│   │   └── power.c         # Reboot/shutdown
│   ├── drivers/            # Additional drivers
│   │   ├── vga.c           # VGA framebuffer
│   │   └── serial.c        # Serial COM1
│   ├── include/            # Headers
│   └── user/               # User code (future)
├── iso/                    # ISO image
├── build/                  # Compiled binaries
└── Makefile                # Build system
```

## Constants & Configuration

- **PAGE_SIZE**: 4096 bytes (4 KB)
- **KERNEL_STACK_SIZE**: 8192 bytes (8 KB)
- **MAX_PROCESSES**: 32 concurrent processes
- **MAX_FILES**: 64 files in VFS
- **MAX_FILE_SIZE**: 1024 bytes (1 KB)
- **PIT Frequency**: 100 Hz (10 ms per tick)
- **Scheduler Quantum**: 10 ticks = 100 ms
