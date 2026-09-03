# Scheduler and Process Management

## Overview

The HumanOS scheduler implements preemptive multitasking with support for Ring 0 (kernel) and Ring 3 (user) processes. It uses a simple round-robin algorithm with a 100 ms quantum.

## Scheduler Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Scheduler Core                            │
│  ┌────────────────────────────────────────────────────────┐ │
│  │  Process Table (MAX_PROCESSES = 32)                    │ │
│  │  - process_t* processes[32]                            │ │
│  │  - current_process (current index)                    │ │
│  │  - process_count (total active)                         │ │
│  └────────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────┐ │
│  │  Context Switch (Assembly)                             │ │
│  │  - scheduler_yield_asm()                               │ │
│  │  - pusha/popa (general registers)                      │ │
│  │  - push/pop segments (DS, ES, FS, GS)                 │ │
│  │  - iretd (restore full context)                        │ │
│  └────────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────┐ │
│  │  Timer Interrupt (PIT)                                 │ │
│  │  - 100 Hz (10 ms per tick)                            │ │
│  │  - Quantum: 10 ticks = 100 ms                          │ │
│  │  - schedule_needed flag                                │ │
│  └────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

## Data Structures

### process_t - Task Control Block

```c
typedef enum {
    PROCESS_READY,      // Ready to execute
    PROCESS_RUNNING,    // Currently executing
    PROCESS_BLOCKED,    // Blocked (waiting)
    PROCESS_TERMINATED  // Terminated
} process_state_t;

typedef struct {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eflags;
} cpu_context_t;

typedef struct {
    uint32_t pid;                           // Process ID
    process_state_t state;                  // Process state
    int exit_code;                          // Exit code
    uint32_t waiting_for_pid;               // PID being waited for (if blocked)
    cpu_context_t context;                  // CPU context
    uint8_t kernel_stack[KERNEL_STACK_SIZE]; // Kernel stack (8 KB)
    void (*entry_point)(void);              // Entry point
    char name[32];                          // Process name
} process_t;
```

### Constants

- **MAX_PROCESSES**: 32 concurrent processes
- **KERNEL_STACK_SIZE**: 8192 bytes (8 KB)
- **SCHEDULER_QUANTUM**: 10 ticks (100 ms)

## Initialization

### scheduler_init()

```c
void scheduler_init(void);
```

Process:
1. Clear process table (all NULL)
2. Reset counters
3. Initialize next_pid = 1
4. Print initialization message

## Process Creation

### scheduler_convert_current_process()

Converts current context (kernel_main) into a scheduler process:

```c
void scheduler_convert_current_process(const char* name);
```

Process:
1. Allocate memory for TCB
2. Initialize KernelMain process (already executing)
3. **Does NOT build stack frame** (uses current kernel stack)
4. Set state = PROCESS_RUNNING
5. Add as process 0 (PID 1)
6. Set current_process = 0

**Important**: KernelMain is special because it's already executing on the original kernel stack. It doesn't need a pre-built stack frame.

### scheduler_add_process()

Adds kernel process (Ring 0):

```c
void scheduler_add_process(void (*entry)(void), const char* name);
```

Process:
1. Allocate memory for TCB
2. Initialize metadata (PID, state, name)
3. **Build stack frame** on kernel_stack:
   - iretd frame: EFLAGS, CS (0x08), EIP (process_wrapper)
   - General registers: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
   - Segments: DS, ES, FS, GS (all 0x10)
4. Set context.esp to frame start
5. Add to process table

**process_wrapper**: Trampoline that calls entry_point() and then process_exit(0) if it returns.

### scheduler_add_user_process()

Adds user process (Ring 3):

```c
void scheduler_add_user_process(void (*entry)(void), const char* name);
```

Process:
1. Allocate memory for TCB
2. Initialize metadata
3. **Build Ring 3 stack frame**:
   - iretd frame with SS and User ESP:
     - SS (0x23)
     - User ESP (kernel_stack - 256)
     - EFLAGS (0x202)
     - CS (0x1B)
     - EIP (entry_point directly)
   - General registers (all 0)
   - User segments (all 0x23)
4. Set context.esp to frame start
5. Add to process table

**Ring 3 Differences**:
- CS = 0x1B (User Code Segment)
- DS/ES/FS/GS = 0x23 (User Data Segment)
- SS and ESP in stack for iretd
- EIP points directly to entry_point (not wrapper)

## Context Switch

### scheduler_yield()

Public wrapper to yield control:

```c
void scheduler_yield(void);
```

### scheduler_yield_asm() - Assembly

Assembly implementation in interrupts.s:

```asm
scheduler_yield_asm:
    pop eax                 ; EAX = Return EIP
    pushfd                  ; Push EFLAGS
    push dword 0x08         ; Push CS (Kernel Code Segment)
    push eax                ; Push Return EIP
    pusha                   ; Save general registers
    push ds
    push es
    push fs
    push gs
    
    ; Load kernel segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp                ; Pass current_esp as argument
    call scheduler_yield_impl
    add esp, 4
    
    test eax, eax
    jz .no_switch
    
    mov esp, eax            ; Switch ESP to new process
    
.no_switch:
    pop gs
    pop fs
    pop es
    pop ds
    popa
    iretd                   ; Return via iretd
```

### scheduler_yield_impl() - C

C implementation:

```c
uint32_t scheduler_yield_impl(uint32_t current_esp);
```

Process:
1. Check if more than 1 process
2. Calculate next process: `(current_process + 1) % process_count`
3. Find next READY process (skip terminated/blocked)
4. If no other READY process, return 0 (no switch)
5. Save current process ESP:
   - If state = RUNNING → change to READY
   - Save context.esp = current_esp
6. Change current_process = next
7. Set next_proc.state = RUNNING
8. **Update TSS esp0** to next process's kernel_stack
9. Return next_proc.context.esp

**TSS Update**: Critical for interrupts to use the correct process stack.

## Preemptive Multitasking

### scheduler_tick()

Called by PIT (IRQ0) every 10 ms:

```c
void scheduler_tick(void);
```

Process:
1. Check if more than 1 process
2. Increment tick_counter
3. Every 10 ticks (100 ms):
   - Reset tick_counter
   - Set schedule_needed = 1

### irq0_stub() - Assembly

PIT interrupt stub in interrupts.s:

```asm
irq0_stub:
    pusha
    push ds
    push es
    push fs
    push gs
    
    ; Load kernel segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    cld
    call pit_handler
    
    cmp byte [schedule_needed], 0
    je .no_schedule
    
    mov byte [schedule_needed], 0
    
    push esp
    call scheduler_yield_impl
    add esp, 4
    
    test eax, eax
    jz .no_schedule
    
    mov esp, eax
    
.no_schedule:
    pop gs
    pop fs
    pop es
    pop ds
    popa
    iretd
```

## Process Management

### process_exit()

Terminates current process:

```c
void process_exit(int exit_code);
```

### process_kill()

Kills process by PID:

```c
int process_kill(uint32_t pid);
```

### process_wait()

Blocks current process until another terminates:

```c
int process_wait(uint32_t pid);
```

## Process States

```
┌─────────────┐
│  PROCESS_   │
│   READY     │◄─────────────┐
└──────┬──────┘              │
       │ scheduler_yield()   │
       ▼                      │
┌─────────────┐              │
│  PROCESS_   │              │
│  RUNNING    │              │
└──────┬──────┘              │
       │ scheduler_tick()    │
       ▼                      │
┌─────────────┐              │
│  PROCESS_   │              │
│   READY     │──────────────┘
└─────────────┘

┌─────────────┐
│  PROCESS_   │
│  BLOCKED    │◄──── process_wait()
└──────┬──────┘
       │ target process terminates
       ▼
┌─────────────┐
│  PROCESS_   │
│   READY     │
└─────────────┘

┌─────────────┐
│  PROCESS_   │
│ TERMINATED  │◄──── process_exit()
└─────────────┘
```

## TSS (Task State Segment)

### Purpose

The TSS is used for:
1. Defining kernel stack (esp0) for interrupts from Ring 3
2. Providing a safe stack when an interrupt occurs in user mode

### Update

```c
void tss_set_kernel_stack(uint32_t kstack);
```

Called in scheduler_yield_impl() before switching process.

## Segment Configuration

### GDT

```
Selector  Descriptor
────────────────────────────────────
0x00      Null Descriptor
0x08      Kernel Code (Ring 0)
0x10      Kernel Data (Ring 0)
0x1B      User Code (Ring 3)
0x23      User Data (Ring 3)
0x2B      TSS (Ring 3)
```
