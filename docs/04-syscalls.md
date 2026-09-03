# Syscalls and Ring 3 Transition

## Overview

HumanOS implements system calls using the `int 0x80` interrupt with DPL=3, allowing Ring 3 (user mode) processes to request kernel services in Ring 0.

## Syscall Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                 User Space (Ring 3)                           │
│  ┌────────────────────────────────────────────────────────┐ │
│  │  sys_write(), sys_read(), sys_yield(), sys_exit()...   │ │
│  │  → int 0x80 (syscall instruction)                      │ │
│  └────────────────────────────────────────────────────────┘ │
└─────────────────────────────┬───────────────────────────────┘
                              │ int 0x80 (DPL=3)
┌─────────────────────────────┴───────────────────────────────┐
│                 Kernel Space (Ring 0)                        │
│  ┌────────────────────────────────────────────────────────┐ │
│  │  IDT Entry 0x80 → syscall_stub (assembly)              │ │
│  │  → isr_common_stub → syscall_handler (C)               │ │
│  └────────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────┐ │
│  │  syscall_handler() - Syscall dispatch                 │ │
│  │  - switch(eax) → specific syscall                      │ │
│  │  - Execute kernel operation                            │ │
│  │  - Return result in eax                               │ │
│  └────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

## IDT Configuration

### Syscall Entry

```c
// idt.c
idt_set_gate(0x80, (uint32_t)syscall_stub, 0x08, 0xEE);
```

- **Vector**: 0x80 (128 decimal)
- **Handler**: syscall_stub (assembly)
- **Selector**: 0x08 (Kernel Code Segment)
- **Flags**: 0xEE
  - Bit 7 (Present): 1
  - Bit 6-5 (DPL): 11 (Ring 3)
  - Bit 4 (Type): 0 (Interrupt Gate)
  - Bit 0 (32-bit): 1

**DPL=3 is critical**: Allows Ring 3 processes to execute int 0x80 without causing General Protection Fault.

## Syscall Stub (Assembly)

### syscall_stub()

```asm
; interrupts.s
syscall_stub:
    push dword 0        ; Fake error code
    push dword 0x80     ; Interrupt number
    pusha
    push ds

    mov ax, 0x10        ; Load kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; Pass pointer to registers_t
    cld
    call syscall_handler
    add esp, 4          ; Clean argument

    pop ds
    popa
    add esp, 8          ; Clean int_no and err_code
    iretd
```

**registers_t**:
```c
typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;
```

## Syscall Handler (C)

### syscall_handler()

```c
void syscall_handler(registers_t* regs);
```

Process:
1. Get syscall number from `regs->eax`
2. Switch to specific syscall
3. Execute operation
4. Return result in `regs->eax`

### Implemented Syscalls

#### SYS_YIELD (0)

```c
case SYS_YIELD:
    scheduler_yield();
    regs->eax = 0;
    break;
```

- **Purpose**: Voluntarily yield control
- **Parameters**: None
- **Return**: 0
- **Wrapper**: `sys_yield()`

#### SYS_EXIT (1)

```c
case SYS_EXIT:
    process_exit((int)regs->ebx);
    break;
```

- **Purpose**: Terminate current process
- **Parameters**: `ebx` = exit_code
- **Return**: Does not return (process terminates)
- **Wrapper**: `sys_exit(int status)`

#### SYS_WRITE (2)

```c
case SYS_WRITE: {
    int fd = (int)regs->ebx;
    const char* str = (const char*)regs->ecx;
    uint32_t count = regs->edx;
    
    if (fd == 1 || fd == 2) { // stdout or stderr
        if (str != NULL) {
            vga_puts("[SYS_WRITE] ", COLOR_LIGHT_GREEN);
            vga_puts(str, COLOR_WHITE);
            if (count > 0 && str[count - 1] != '\n') {
                vga_putc('\n', COLOR_WHITE);
            }
        }
        regs->eax = count;
    } else {
        regs->eax = (uint32_t)-1;
    }
    break;
}
```

- **Purpose**: Write to file descriptor
- **Parameters**: 
  - `ebx` = fd (1=stdout, 2=stderr)
  - `ecx` = string pointer
  - `edx` = length
- **Return**: bytes written or -1 if error
- **Wrapper**: `sys_write(int fd, const char* str, uint32_t count)`

#### SYS_READ (3)

```c
case SYS_READ: {
    int fd = (int)regs->ebx;
    char* buf = (char*)regs->ecx;
    uint32_t count = regs->edx;
    extern char keyboard_getchar(void);
    
    if (fd == 0 && buf != NULL && count > 0) { // stdin
        uint32_t read_bytes = 0;
        while (read_bytes < count) {
            char c = keyboard_getchar();
            if (c != 0) {
                buf[read_bytes++] = c;
                if (c == '\n' || c == '\r') break;
            } else {
                break;
            }
        }
        regs->eax = read_bytes;
    } else {
        regs->eax = 0;
    }
    break;
}
```

- **Purpose**: Read from file descriptor
- **Parameters**:
  - `ebx` = fd (0=stdin)
  - `ecx` = buffer
  - `edx` = max length
- **Return**: bytes read
- **Wrapper**: `sys_read(int fd, char* buf, uint32_t count)`

#### SYS_MALLOC (4)

```c
case SYS_MALLOC: {
    uint32_t size = regs->ebx;
    regs->eax = (uint32_t)kmalloc(size);
    break;
}
```

- **Purpose**: Allocate dynamic memory
- **Parameters**: `ebx` = size in bytes
- **Return**: pointer to memory or NULL
- **Wrapper**: `sys_malloc(uint32_t size)`

#### SYS_GETPID (5)

```c
case SYS_GETPID:
    regs->eax = scheduler_get_current_pid();
    break;
```

- **Purpose**: Get current process PID
- **Parameters**: None
- **Return**: Current PID
- **Wrapper**: `sys_getpid()`

## User Wrappers

The wrappers are functions that execute `int 0x80` with correct parameters:

### sys_yield()

```c
void sys_yield(void) {
    __asm__ __volatile__("int $0x80" : : "a"(SYS_YIELD));
}
```

### sys_exit()

```c
void sys_exit(int status) {
    __asm__ __volatile__("int $0x80" : : "a"(SYS_EXIT), "b"(status));
}
```

### sys_write()

```c
int sys_write(int fd, const char* str, uint32_t count) {
    int ret;
    __asm__ __volatile__(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_WRITE), "b"(fd), "c"(str), "d"(count)
        : "memory"
    );
    return ret;
}
```

### sys_read()

```c
int sys_read(int fd, char* buf, uint32_t count) {
    int ret;
    __asm__ __volatile__(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_READ), "b"(fd), "c"(buf), "d"(count)
        : "memory"
    );
    return ret;
}
```

### sys_malloc()

```c
void* sys_malloc(uint32_t size) {
    void* ret;
    __asm__ __volatile__(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_MALLOC), "b"(size)
        : "memory"
    );
    return ret;
}
```

### sys_getpid()

```c
uint32_t sys_getpid(void) {
    uint32_t ret;
    __asm__ __volatile__(
        "int $0x80"
        : "=a"(ret)
        : "a"(SYS_GETPID)
        : "memory"
    );
    return ret;
}
```

## Calling Convention

### Register Parameters

```
EAX: Syscall number
EBX: First parameter
ECX: Second parameter
EDX: Third parameter
```

### Return Register

```
EAX: Return value
```

### Clobber List

Wrappers specify `"memory"` in clobber list because:
- The syscall may modify memory
- Compiler should not assume memory is unchanged

## Ring 3 → Ring 0 Transition

### Execution Flow

1. **Ring 3 Process** calls wrapper (e.g., sys_write)
2. **Wrapper** loads parameters in registers
3. **Wrapper** executes `int 0x80`
4. **CPU** verifies IDT entry DPL (3) → allowed
5. **CPU** switches to Ring 0:
   - SS:ESP saved on stack
   - CS:EIP saved on stack
   - EFLAGS saved on stack
   - CS changes to 0x08 (Kernel Code)
   - SS changes to 0x10 (Kernel Data)
6. **syscall_stub** executes:
   - Saves registers
   - Loads kernel segments
   - Calls syscall_handler
7. **syscall_handler** executes operation
8. **syscall_stub** restores registers
9. **iretd** restores Ring 3 context:
   - Pop SS:ESP
   - Pop EFLAGS
   - Pop CS:EIP
   - Return to Ring 3

## Security and Validation

### Parameter Validation

The kernel should validate Ring 3 parameters:
1. **Pointers**: Verify they point to valid user memory
2. **Lengths**: Verify they don't exceed limits
3. **FDs**: Verify descriptor is valid
4. **Permissions**: Verify process has permissions

### Current Implementation

Current implementation has basic validation:
- sys_write verifies fd == 1 or fd == 2
- sys_read verifies fd == 0
- sys_malloc doesn't validate return pointer
- sys_exit doesn't validate exit code

## Future Syscalls

### Planned

- **SYS_OPEN**: Open file
- **SYS_CLOSE**: Close file
- **SYS_LSEEK**: Seek in file
- **SYS_FORK**: Create child process
- **SYS_EXEC**: Execute program
- **SYS_WAITPID**: Wait for child process
- **SYS_KILL**: Send signal to process
- **SYS_GETTIME**: Get system time
- **SYS_SLEEP**: Sleep process
- **SYS_PIPE**: Create IPC pipe
