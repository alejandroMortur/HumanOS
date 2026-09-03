# HumanOS v0.0.1 🚀

> **A 32-bit operating system for x86 (i686) architecture with preemptive multitasking, virtual memory management, and Ring 3 user mode transition.**

---

## 📌 Project Overview

**HumanOS** is a hobby operating system kernel built from scratch for x86 (i686) architecture. It features a complete kernel with preemptive multitasking, virtual memory management, a virtual file system, hardware drivers, and an interactive user shell running in Ring 3.

---

## 🛠️ Implemented Features

### 1. Bootstrapping & Core
- ✅ **Multiboot1/2 Compliant Header** (GRUB/Limine)
- ✅ **GDT** with Kernel/User segments and TSS
- ✅ **IDT** with 32 exception ISRs
- ✅ **PIC 8259** remapping (IRQs 32-47)

### 2. Memory Management
- ✅ **PMM** (Physical Memory Manager) - Bitmap allocator (up to 4 GB)
- ✅ **VMM** (Virtual Memory Manager) - Identity-mapped paging
- ✅ **Heap** - Dynamic memory allocator (kmalloc/kfree)

### 3. Process & Task Management
- ✅ **Preemptive Multitasking** (PIT Timer at 100 Hz)
- ✅ **Context Switch** (assembly implementation)
- ✅ **Ring 0 & Ring 3 Processes**
- ✅ **Task Control Block (TCB)** with state, context, and metadata
- ✅ **Process states**: READY, RUNNING, BLOCKED, TERMINATED

### 4. Syscalls & IPC
- ✅ **int 0x80** with DPL=3 (accessible from Ring 3)
- ✅ **Implemented syscalls**: yield, exit, write, read, malloc, free, getpid

### 5. Hardware Drivers
- ✅ **PIT** (Programmable Interval Timer) - 100 Hz
- ✅ **PS/2 Keyboard** - Basic input driver
- ✅ **VGA Text Mode** - 80x25 with 16 colors
- ✅ **Serial COM1** - Debug logs
- ✅ **ATA PIO** - Hard disk driver with LBA28

### 6. File System
- ✅ **VFS** (Virtual File System)
- ✅ **ATA disk storage** (sector 100 metadata, sectors 101+ data)
- ✅ **Operations**: create, write, read, list

### 7. Userland & Shell
- ✅ **Interactive Shell** in Ring 3
- ✅ **Commands**: help, ls, cat, touch, write, diskinfo, sysinfo, whoami, hostname, uname, clear, ps, stress, reboot, shutdown, exit
- ✅ **Command history** (10 commands)
- ✅ **Tab autocomplete**
- ✅ **ELF Loader** for user-space binaries

### 8. Additional Features
- ✅ **CPU Info** (CPUID vendor, model, cores, brand string)
- ✅ **Power Management** (reboot, shutdown)
- ✅ **Stress test** command for stability verification

---

## 🚦 Current Status

| Feature / Subsystem | Status |
| :--- | :--- |
| **Bootloader Integration** | 🟢 Complete |
| **Memory Management** | 🟢 Complete |
| **Preemptive Scheduler** | 🟢 Complete |
| **Ring 3 Userland** | 🟢 Complete |
| **Syscalls** | 🟢 Complete |
| **Hardware Drivers** | 🟢 Complete |
| **File System (VFS)** | 🟢 Complete |
| **Interactive Shell** | � Complete |
| **Documentation** | 🟢 Complete |

---

## 📚 Documentation

Complete technical documentation is available in the `docs/` folder:

- **[docs/README.md](docs/README.md)** - Documentation index
- **[docs/01-architecture.md](docs/01-architecture.md)** - System architecture
- **[docs/02-memory-management.md](docs/02-memory-management.md)** - Memory management
- **[docs/03-scheduler.md](docs/03-scheduler.md)** - Scheduler and processes
- **[docs/04-syscalls.md](docs/04-syscalls.md)** - Syscalls and Ring 3 transition
- **[docs/05-drivers.md](docs/05-drivers.md)** - Hardware drivers
- **[docs/06-filesystem.md](docs/06-filesystem.md)** - File system (VFS)
- **[docs/07-userland.md](docs/07-userland.md)** - Userland and shell
- **[docs/CHECKLIST.MD](docs/CHECKLIST.MD)** - v0.0.1 implementation checklist
- **[CHECKLIST.MD](CHECKLIST.MD)** - v2.0 implementation roadmap

---

## 🔧 Building & Running

### Prerequisites
- `gcc` cross-compiler (target `i686-elf`)
- `nasm` assembler
- `ld` linker
- GRUB or Limine bootloader
- QEMU or VirtualBox for emulation

### Build
```bash
make clean
make
```

This generates:
- `build/kernel.bin` - Kernel binary
- `build/HumanOS.iso` - Bootable ISO image

### Running in QEMU
```bash
qemu-system-i386 -cdrom build/HumanOS.iso
```

### Running in VirtualBox

**First time setup:**

1. Create a new VM with:
   - **OS Type**: Other/Unknown (32-bit)
   - **Memory**: 32 MB minimum
   - **Storage**: Create new disk (skip for now)

2. After creating the VM, go to Settings → Storage:
   - Attach `build/HumanOS.iso` as CD/DVD (IDE Secondary Master)
   - Attach `build/hdd.img` as Hard Disk (IDE Primary Master)
     - Click "Add" → "Choose existing disk"
     - Select `build/hdd.img` from the project folder
     - Make sure it's set as "IDE Primary Master"

3. Start the VM

**Note**: The `hdd.img` file is created automatically when you run `make`. It's a 10 MB persistent disk that stores your files between reboots.

---

## 🗺️ Roadmap

### v0.0.1 (Current) ✅
- Core kernel with preemptive multitasking
- Memory management (PMM, VMM, Heap)
- Hardware drivers (PIT, Keyboard, VGA, Serial, ATA)
- File system (VFS)
- User shell in Ring 3
- Complete documentation

### v2.0 (Planned)
See [CHECKLIST.MD](CHECKLIST.MD) for the complete v2.0 roadmap including:
- Enhanced memory management (paging, swap, CoW)
- Advanced process management (fork, exec, signals, threads)
- Enhanced file system (directories, permissions, pipes)
- Networking stack (Ethernet, TCP/IP, sockets)
- Graphics & UI (framebuffer, window manager)
- Security (users, permissions, ASLR)
- And much more...

---

## 💖 Philosophy & Vision

HumanOS is designed as a learning project to understand operating system internals. Every component is written from scratch to provide complete transparency between hardware interfaces and system orchestration. The project emphasizes clean code, comprehensive documentation, and progressive feature development.

---

## 📝 License

This project is open source and available for educational purposes.

---

**Version:** v0.0.1  
**Architecture:** x86 (i686) 32-bit  
**Mode:** Protected with paging  
**Rings:** Ring 0 (kernel) and Ring 3 (user)