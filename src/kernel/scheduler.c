#include "scheduler.h"
#include "heap.h"
#include "vga.h"
#include "gdt.h"
#include <stdint.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

// Procesos del scheduler
static process_t* processes[MAX_PROCESSES];
static uint32_t current_process = 0;
static uint32_t process_count = 0;
static uint32_t next_pid = 1;
volatile uint8_t schedule_needed = 0;

// Función externa para cambio de contexto (assembly)
extern void context_switch(cpu_context_t* old_context, cpu_context_t* new_context);
extern void task_switch(uint32_t* current_esp_ptr, uint32_t new_esp);
extern void scheduler_yield_asm(void);

// Trampolín de ejecución para envolver entry_point y llamar a process_exit(0) si retorna de forma normal
static void process_wrapper(void) {
    process_t* current = processes[current_process];
    if (current != NULL && current->entry_point != NULL) {
        current->entry_point();
    }
    process_exit(0);
}

// Inicializar scheduler
void scheduler_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processes[i] = NULL;
    }
    current_process = 0;
    process_count = 0;
    next_pid = 1;
    
    vga_puts("[SCHEDULER] Initialized\n", 0x1A);
}

// Convertir el contexto actual (kernel_main) en un proceso del scheduler
void scheduler_convert_current_process(const char* name) {
    if (process_count >= MAX_PROCESSES) {
        vga_puts("[SCHEDULER] Max processes reached\n", 0x1C);
        return;
    }
    
    // Asignar memoria para el proceso
    process_t* proc = (process_t*)kmalloc(sizeof(process_t));
    if (proc == NULL) {
        vga_puts("[SCHEDULER] Failed to allocate process\n", 0x1C);
        return;
    }
    
    // Inicializar proceso KernelMain (ya en ejecución en el stack del kernel)
    proc->pid = next_pid++;
    proc->state = PROCESS_RUNNING;
    proc->entry_point = NULL;
    proc->exit_code = 0;
    proc->waiting_for_pid = 0;
    
    // Copiar nombre
    int i = 0;
    while (name[i] != '\0' && i < 31) {
        proc->name[i] = name[i];
        i++;
    }
    proc->name[i] = '\0';
    
    proc->context.esp = 0; // Se actualizará al desapropiar o ceder
    proc->context.eflags = 0x202;
    
    // Agregar como proceso 0
    processes[0] = proc;
    process_count = 1;
    current_process = 0;
    
    vga_puts("[SCHEDULER] Converted current process: ", 0x1A);
    vga_puts(proc->name, 0x1A);
    vga_putc('\n', 0x1A);
}

// Agregar proceso al scheduler (Modo Kernel - Ring 0)
void scheduler_add_process(void (*entry)(void), const char* name) {
    if (process_count >= MAX_PROCESSES) {
        vga_puts("[SCHEDULER] Max processes reached\n", 0x1C);
        return;
    }
    
    process_t* proc = (process_t*)kmalloc(sizeof(process_t));
    if (proc == NULL) {
        vga_puts("[SCHEDULER] Failed to allocate process\n", 0x1C);
        return;
    }
    
    proc->pid = next_pid++;
    proc->state = PROCESS_READY;
    proc->entry_point = entry;
    proc->exit_code = 0;
    proc->waiting_for_pid = 0;
    
    int i = 0;
    while (name[i] != '\0' && i < 31) {
        proc->name[i] = name[i];
        i++;
    }
    proc->name[i] = '\0';
    
    uint32_t* stack_top = (uint32_t*)&proc->kernel_stack[KERNEL_STACK_SIZE];
    
    // 1. Marco para iretd (Kernel CS: 0x08)
    stack_top--; *stack_top = 0x202;                      // EFLAGS (Interrupts enabled)
    stack_top--; *stack_top = KERNEL_CS;                  // CS (Kernel Code Segment 0x08)
    stack_top--; *stack_top = (uint32_t)process_wrapper;  // EIP (Trampolín de ejecución)
    
    // 2. Registros generales para popa
    stack_top--; *stack_top = 0;  // EAX
    stack_top--; *stack_top = 0;  // ECX
    stack_top--; *stack_top = 0;  // EDX
    stack_top--; *stack_top = 0;  // EBX
    stack_top--; *stack_top = 0;  // ESP (dummy)
    stack_top--; *stack_top = 0;  // EBP
    stack_top--; *stack_top = 0;  // ESI
    stack_top--; *stack_top = 0;  // EDI
    
    // 3. Registros de segmento (Kernel DS: 0x10)
    stack_top--; *stack_top = KERNEL_DS; // DS
    stack_top--; *stack_top = KERNEL_DS; // ES
    stack_top--; *stack_top = KERNEL_DS; // FS
    stack_top--; *stack_top = KERNEL_DS; // GS
    
    proc->context.esp = (uint32_t)stack_top;
    proc->context.eflags = 0x202;
    
    processes[process_count] = proc;
    process_count++;
    
    vga_puts("[SCHEDULER] Added Kernel Process (Ring 0): ", 0x1A);
    vga_puts(proc->name, 0x1A);
    vga_putc('\n', 0x1A);
}

// Agregar proceso de usuario al scheduler (Modo Usuario - Ring 3)
void scheduler_add_user_process(void (*entry)(void), const char* name) {
    if (process_count >= MAX_PROCESSES) {
        vga_puts("[SCHEDULER] Max processes reached\n", 0x1C);
        return;
    }
    
    process_t* proc = (process_t*)kmalloc(sizeof(process_t));
    if (proc == NULL) {
        vga_puts("[SCHEDULER] Failed to allocate process\n", 0x1C);
        return;
    }
    
    proc->pid = next_pid++;
    proc->state = PROCESS_READY;
    proc->entry_point = entry;
    proc->exit_code = 0;
    proc->waiting_for_pid = 0;
    
    int i = 0;
    while (name[i] != '\0' && i < 31) {
        proc->name[i] = name[i];
        i++;
    }
    proc->name[i] = '\0';
    
    uint32_t* stack_top = (uint32_t*)&proc->kernel_stack[KERNEL_STACK_SIZE];
    
    // Marco iretd para Ring 3 (requiere SS y User ESP en la pila)
    stack_top--; *stack_top = USER_DS;                     // User SS (0x23)
    stack_top--; *stack_top = (uint32_t)&proc->kernel_stack[KERNEL_STACK_SIZE - 256]; // User ESP
    stack_top--; *stack_top = 0x202;                       // EFLAGS (Interrupts enabled)
    stack_top--; *stack_top = USER_CS;                     // User CS (0x1B = 0x18 | 3)
    stack_top--; *stack_top = (uint32_t)entry;             // EIP (Direct User Function Entry)
    
    // Registros generales para popa
    stack_top--; *stack_top = 0;  // EAX
    stack_top--; *stack_top = 0;  // ECX
    stack_top--; *stack_top = 0;  // EDX
    stack_top--; *stack_top = 0;  // EBX
    stack_top--; *stack_top = 0;  // ESP (dummy)
    stack_top--; *stack_top = 0;  // EBP
    stack_top--; *stack_top = 0;  // ESI
    stack_top--; *stack_top = 0;  // EDI
    
    // Registros de segmento de usuario (0x23)
    stack_top--; *stack_top = USER_DS; // DS (0x23)
    stack_top--; *stack_top = USER_DS; // ES (0x23)
    stack_top--; *stack_top = USER_DS; // FS (0x23)
    stack_top--; *stack_top = USER_DS; // GS (0x23)
    
    proc->context.esp = (uint32_t)stack_top;
    proc->context.eflags = 0x202;
    
    processes[process_count] = proc;
    process_count++;
    
    vga_puts("[SCHEDULER] Added User Process (Ring 3): ", COLOR_LIGHT_GREEN);
    vga_puts(proc->name, COLOR_LIGHT_GREEN);
    vga_putc('\n', COLOR_LIGHT_GREEN);
}

// Ceder control al scheduler (wrapper público)
void scheduler_yield(void) {
    if (process_count <= 1) {
        return;
    }
    scheduler_yield_asm();
}

// Implementación interna del scheduler (llamada desde assembly)
uint32_t scheduler_yield_impl(uint32_t current_esp) {
    if (process_count <= 1) {
        return 0;
    }
    
    uint32_t next = (current_process + 1) % process_count;
    
    uint32_t count = 0;
    while (count < process_count) {
        if (processes[next] != NULL && processes[next]->state == PROCESS_READY) {
            break;
        }
        next = (next + 1) % process_count;
        count++;
    }
    
    if (count >= process_count || next == current_process) {
        return 0;  // No hay otro proceso READY para cambiar
    }
    
    process_t* current_proc = processes[current_process];
    process_t* next_proc = processes[next];
    
    if (current_proc != NULL && current_proc->state == PROCESS_RUNNING) {
        current_proc->state = PROCESS_READY;
        current_proc->context.esp = current_esp;
    } else if (current_proc != NULL) {
        current_proc->context.esp = current_esp;
    }
    
    current_process = next;
    next_proc->state = PROCESS_RUNNING;
    
    // Actualizar esp0 del TSS al stack del kernel del siguiente proceso
    tss_set_kernel_stack((uint32_t)&next_proc->kernel_stack[KERNEL_STACK_SIZE]);
    
    return next_proc->context.esp;
}

// Tick del scheduler (llamado por PIT)
void scheduler_tick(void) {
    if (process_count <= 1) {
        return;
    }
    
    static uint32_t tick_counter = 0;
    tick_counter++;
    
    // Hacer cambio de contexto cada 10 ticks (100 ms)
    if (tick_counter >= 10) {
        tick_counter = 0;
        schedule_needed = 1;
    }
}

// Obtener PID del proceso actual
uint32_t scheduler_get_current_pid(void) {
    if (process_count == 0) {
        return 0;
    }
    return processes[current_process]->pid;
}

// Obtener puntero al proceso actual
process_t* scheduler_get_current_process(void) {
    if (process_count == 0) {
        return NULL;
    }
    return processes[current_process];
}

// Finalizar de forma limpia el proceso actual
void process_exit(int exit_code) {
    process_t* current = processes[current_process];
    if (current == NULL) return;
    
    current->state = PROCESS_TERMINATED;
    current->exit_code = exit_code;
    
    vga_puts("[SCHEDULER] Process '", 0x1E);
    vga_puts(current->name, 0x1E);
    vga_puts("' (PID ", 0x1E);
    
    char buf[16];
    int count = current->pid;
    int j = 0;
    while (count > 0 && j < 15) {
        buf[j++] = '0' + (count % 10);
        count /= 10;
    }
    for (int k = j - 1; k >= 0; k--) vga_putc(buf[k], 0x1E);
    
    vga_puts(") exited with code ", 0x1E);
    count = exit_code < 0 ? -exit_code : exit_code;
    j = 0;
    if (count == 0) buf[j++] = '0';
    while (count > 0 && j < 15) {
        buf[j++] = '0' + (count % 10);
        count /= 10;
    }
    if (exit_code < 0) vga_putc('-', 0x1E);
    for (int k = j - 1; k >= 0; k--) vga_putc(buf[k], 0x1E);
    vga_puts("\n", 0x1E);
    
    // Despertar a los procesos en PROCESS_BLOCKED que esperaban a este PID
    for (uint32_t i = 0; i < process_count; i++) {
        if (processes[i] != NULL && processes[i]->state == PROCESS_BLOCKED && processes[i]->waiting_for_pid == current->pid) {
            processes[i]->state = PROCESS_READY;
            processes[i]->waiting_for_pid = 0;
        }
    }
    
    // Ceder el control inmediatamente a otro proceso
    scheduler_yield();
}

// Cancelar un proceso dinámicamente por su PID
int process_kill(uint32_t pid) {
    for (uint32_t i = 0; i < process_count; i++) {
        if (processes[i] != NULL && processes[i]->pid == pid && processes[i]->state != PROCESS_TERMINATED) {
            processes[i]->state = PROCESS_TERMINATED;
            processes[i]->exit_code = -1;
            
            vga_puts("[SCHEDULER] Process '", 0x1C);
            vga_puts(processes[i]->name, 0x1C);
            vga_puts("' killed.\n", 0x1C);
            
            // Despertar a procesos que esperaban por él
            for (uint32_t j = 0; j < process_count; j++) {
                if (processes[j] != NULL && processes[j]->state == PROCESS_BLOCKED && processes[j]->waiting_for_pid == pid) {
                    processes[j]->state = PROCESS_READY;
                    processes[j]->waiting_for_pid = 0;
                }
            }
            
            if (i == current_process) {
                scheduler_yield();
            }
            return 0;
        }
    }
    return -1;
}

// Bloquear el proceso actual hasta que finalice el proceso objetivo (PID)
int process_wait(uint32_t pid) {
    process_t* current = processes[current_process];
    if (current == NULL) return -1;
    
    process_t* target = NULL;
    for (uint32_t i = 0; i < process_count; i++) {
        if (processes[i] != NULL && processes[i]->pid == pid) {
            target = processes[i];
            break;
        }
    }
    
    if (target == NULL) return -1;
    
    if (target->state == PROCESS_TERMINATED) {
        return target->exit_code;
    }
    
    current->state = PROCESS_BLOCKED;
    current->waiting_for_pid = pid;
    
    while (target->state != PROCESS_TERMINATED) {
        scheduler_yield();
    }
    
    return target->exit_code;
}

// Verificar si se necesita cambio de contexto (llamado desde loop principal)
void scheduler_check(void) {
    if (schedule_needed) {
        schedule_needed = 0;
        scheduler_yield();
    }
}

// Terminar todos los procesos activos excepto el proceso invocador de apagado
void scheduler_terminate_all(void) {
    uint32_t curr_pid = scheduler_get_current_pid();
    for (uint32_t i = 0; i < process_count; i++) {
        if (processes[i] != NULL && processes[i]->pid != curr_pid) {
            processes[i]->state = PROCESS_TERMINATED;
        }
    }
}
