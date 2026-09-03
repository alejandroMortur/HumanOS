#include "syscall.h"
#include "scheduler.h"
#include "vga.h"
#include "heap.h"
#include <stdint.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

// Manejador centralizado de llamadas al sistema (int 0x80)
void syscall_handler(registers_t* regs) {
    uint32_t syscall_num = regs->eax;
    
    switch (syscall_num) {
        case SYS_YIELD:
            scheduler_yield();
            regs->eax = 0;
            break;
            
        case SYS_EXIT:
            process_exit((int)regs->ebx);
            break;
            
        case SYS_WRITE: {
            int fd = (int)regs->ebx;
            const char* str = (const char*)regs->ecx;
            uint32_t count = regs->edx;
            
            if (fd == 1 || fd == 2) { // stdout o stderr
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
        
        case SYS_MALLOC: {
            uint32_t size = regs->ebx;
            regs->eax = (uint32_t)kmalloc(size);
            break;
        }
        
        case SYS_GETPID:
            regs->eax = scheduler_get_current_pid();
            break;
            
        default:
            vga_puts("[SYSCALL] Unknown syscall: ", 0x1C);
            regs->eax = (uint32_t)-1;
            break;
    }
}

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

// Wrappers del lado del procesocliente (ejecutan int 0x80)

void sys_yield(void) {
    __asm__ __volatile__("int $0x80" : : "a"(SYS_YIELD));
}

void sys_exit(int status) {
    __asm__ __volatile__("int $0x80" : : "a"(SYS_EXIT), "b"(status));
}

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
