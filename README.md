# HumanOS (Radium Line) 🚀

> **A custom, high-performance operating system kernel built with precision engineering, modular architecture, and modern multi-threading design.**

---

## 📌 Project Overview

**HumanOS** is a custom operating system kernel engineered with a strong focus on technical clarity, low-level control, and progressive design. Built to run on x86 targets, it serves as the core foundational layer for custom system execution, featuring modular hardware abstraction, tailored memory management, and structured process execution.

---

## 🛠️ Implemented Architecture & Features

### 1. Bootstrapping & Core Execution
* **Multiboot Entry Point:** Clean boot sequence handling transitions into custom execution.
* **CPU & Control Diagnostics:** Interrupt setup and Low-Level Descriptor Table (GDT/IDT) initialization.
* **Core Kernel Initialization (`kernel_main`):** Central entry point coordinating memory, register states, and task registration.

### 2. Task Scheduling & Context Switching
* **Cooperative Multitasking Engine:** Fully functional voluntary process yield system via assembly-level context switching (`scheduler_yield_asm`).
* **Task Control Block (TCB) Management:** Process lifecycle handling, internal queue structures, and task registration.
* **Interrupt Frame Alignment:** Handled fake interrupt frame generation (`iretd`) for task switching to guarantee stack safety across process switches.
* *(In Progress)* **Preemptive Multitasking:** Temporarily shifted back to cooperative mode to refine stack frame initialization before re-enabling timer-driven preemptive switching.

### 3. Memory & System Infrastructure
* **Stack Alignment & Frame Construction:** Dynamic stack initialization per process, ensuring ABI compliance and CPU-level register preservation.
* **Target Architecture Support:** Configured for x86 architectures with dedicated long-mode / 64-bit virtualization setup paths.

---

## 🚦 Current Status & Roadmap

| Feature / Subsystem | Status | Details |
| :--- | :--- | :--- |
| **Bootloader Integration** | 🟢 Complete | Boots smoothly into `kernel_main` |
| **Cooperative Scheduler** | 🟢 Active | Yields execution safely across active tasks |
| **Fake Stack Frame Constructor** | 🟡 Refining | Aligning `kernel_main` and process stack frames for `iretd` |
| **Preemptive Multitasking** | 🟡 Paused | Pending stack frame verification on cooperative mode |
| **Memory Paging / Virtual Memory** | 🔴 Planned | PML4 / Page Table initialization |

---

## 🔧 Building & Running (VirtualBox / QEMU)

### Prerequisites
* `gcc` / `clang` cross-compiler (target `i686-elf` or `x86_64-elf`)
* `nasm` / `gas` assembler
* VirtualBox or QEMU for emulation

### Running in VirtualBox
> **Important Configuration Note:**
> Ensure your VirtualBox VM profile is configured to **Other/Unknown (64-bit)** if executing 64-bit long-mode routines, or ensure Hyper-V (`NEM` mode) is disabled on Windows host environments to prevent execution slowdowns or CPU exceptions.

---

## 💖 Philosophy & Vision

HumanOS is designed around principles of emotional UI coherence, continuous improvement (*Kaizen*), and bulletproof technical authority. Every abstraction layer is written to maintain total transparency between hardware interfaces and upper system orchestration.