#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "isr.h"

// Números de llamadas al sistema (Syscalls)
#define SYS_YIELD  0
#define SYS_EXIT   1
#define SYS_WRITE  2
#define SYS_MALLOC 3
#define SYS_GETPID 4
#define SYS_READ   5
#define SYS_FREE   6

// Despachador de llamadas al sistema en C
void syscall_handler(registers_t* regs);

// Wrappers cliente que ejecutan int $0x80
void sys_yield(void);
void sys_exit(int status);
int sys_write(int fd, const char* str, uint32_t count);
int sys_read(int fd, char* buf, uint32_t count);
void* sys_malloc(uint32_t size);
uint32_t sys_getpid(void);
void sys_free(void* ptr);

#endif
