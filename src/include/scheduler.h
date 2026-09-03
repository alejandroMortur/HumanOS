#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#define MAX_PROCESSES 16
#define KERNEL_STACK_SIZE 4096

// Estados de proceso
typedef enum {
    PROCESS_RUNNING,
    PROCESS_READY,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;

// Estructura de contexto de CPU
typedef struct {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t eflags;
    uint32_t eip;
} __attribute__((packed)) cpu_context_t;

// Estructura de proceso
typedef struct {
    uint32_t pid;
    process_state_t state;
    int exit_code;
    uint32_t waiting_for_pid;
    cpu_context_t context;
    uint8_t kernel_stack[KERNEL_STACK_SIZE];
    void (*entry_point)(void);
    char name[32];
} process_t;

// Funciones del scheduler y gestión de procesos
void scheduler_init(void);
void scheduler_convert_current_process(const char* name);
void scheduler_add_process(void (*entry)(void), const char* name);
void scheduler_add_user_process(void (*entry)(void), const char* name);
void scheduler_yield(void);
uint32_t scheduler_yield_impl(uint32_t current_esp);
void scheduler_tick(void);
void scheduler_check(void);
uint32_t scheduler_get_current_pid(void);
process_t* scheduler_get_current_process(void);

// API de gestión de procesos
void process_exit(int exit_code);
int process_kill(uint32_t pid);
int process_wait(uint32_t pid);

#endif
