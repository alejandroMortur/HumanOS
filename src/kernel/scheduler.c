#include "scheduler.h"
#include "heap.h"
#include "vga.h"
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
    
    // Inicializar proceso
    proc->pid = next_pid++;
    proc->state = PROCESS_READY;
    proc->entry_point = NULL;  // No tiene punto de entrada, ya está ejecutándose
    
    // Copiar nombre
    int i = 0;
    while (name[i] != '\0' && i < 31) {
        proc->name[i] = name[i];
        i++;
    }
    proc->name[i] = '\0';
    
    // Obtener ESP actual y construir marco de interrupción falso
    uint32_t current_esp;
    uint32_t current_eip;
    uint32_t current_eflags;
    
    __asm__ __volatile__(
        "mov %%esp, %0\n"
        "mov $1f, %1\n"
        "pushf\n"
        "pop %2\n"
        "1:\n"
        : "=r"(current_esp), "=r"(current_eip), "=r"(current_eflags)
    );
    
    // Ajustar ESP para dejar espacio para el marco de interrupción falso
    // El stack debe tener (de abajo a arriba): GS, FS, ES, DS, EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI, EIP, CS, EFLAGS, ESP, SS
    // Esto es lo que scheduler_yield_asm espera: push gs, fs, es, ds, pushad, luego iretd
    uint32_t* stack_ptr = (uint32_t*)current_esp;
    stack_ptr -= 17;  // Dejar espacio para 17 valores
    
    // Construir marco de interrupción falso (orden correcto para scheduler_yield_asm)
    // Primero los registros de segmento (push order: gs, fs, es, ds)
    stack_ptr[0] = 0x10;  // GS
    stack_ptr[1] = 0x10;  // FS
    stack_ptr[2] = 0x10;  // ES
    stack_ptr[3] = 0x10;  // DS
    
    // Luego pushad (EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)
    stack_ptr[4] = 0;   // EAX
    stack_ptr[5] = 0;   // ECX
    stack_ptr[6] = 0;   // EDX
    stack_ptr[7] = 0;   // EBX
    stack_ptr[8] = current_esp;  // ESP (valor original antes del ajuste)
    stack_ptr[9] = 0;   // EBP
    stack_ptr[10] = 0;  // ESI
    stack_ptr[11] = 0;  // EDI
    
    // Finalmente el marco para iretd (EIP, CS, EFLAGS, ESP, SS)
    // ESP debe apuntar al stack original (antes de reservar espacio para el marco)
    stack_ptr[12] = current_eip;  // EIP
    stack_ptr[13] = 0x08;  // CS (kernel code segment)
    stack_ptr[14] = current_eflags | 0x202;  // EFLAGS (con interrupts enabled)
    stack_ptr[15] = current_esp;  // ESP (valor original antes del ajuste)
    stack_ptr[16] = 0x10;  // SS (data segment)
    
    // Configurar el contexto del proceso
    // ESP debe apuntar al inicio del marco de iretd (SS), no al fondo del stack
    proc->context.esp = (uint32_t)(stack_ptr + 16);  // Apuntar a SS
    
    // Limpiar el resto del contexto
    proc->context.edi = 0;
    proc->context.esi = 0;
    proc->context.ebp = 0;
    proc->context.ebx = 0;
    proc->context.edx = 0;
    proc->context.ecx = 0;
    proc->context.eax = 0;
    proc->context.eflags = current_eflags | 0x202;
    
    // Actualizar ESP real del procesador
    __asm__ __volatile__("mov %0, %%esp" : : "r"(stack_ptr));
    
    // Agregar a la lista como primer proceso
    processes[process_count] = proc;
    process_count++;
    current_process = 0;
    
    vga_puts("[SCHEDULER] Converted current process: ", 0x1A);
    vga_puts(proc->name, 0x1A);
    vga_putc('\n', 0x1A);
}

// Agregar proceso al scheduler
void scheduler_add_process(void (*entry)(void), const char* name) {
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
    
    // Inicializar proceso
    proc->pid = next_pid++;
    proc->state = PROCESS_READY;
    proc->entry_point = entry;
    
    // Copiar nombre
    int i = 0;
    while (name[i] != '\0' && i < 31) {
        proc->name[i] = name[i];
        i++;
    }
    proc->name[i] = '\0';
    
    // Configurar stack con marco de interrupción falso
    uint32_t* stack_top = (uint32_t*)&proc->kernel_stack[KERNEL_STACK_SIZE];
    
    // Construir marco de interrupción falso (de arriba a abajo en el stack)
    stack_top--;  // SS (no usado en ring 0)
    *stack_top = 0x10;  // Data segment
    stack_top--;  // ESP (no usado)
    *stack_top = 0;
    stack_top--;  // EFLAGS (con interrupts enabled)
    *stack_top = 0x202;
    stack_top--;  // CS (kernel code segment)
    *stack_top = 0x08;
    stack_top--;  // EIP (punto de entrada del proceso)
    *stack_top = (uint32_t)entry;
    
    // Registros generales (pusha order: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)
    stack_top--;  // EDI
    *stack_top = 0;
    stack_top--;  // ESI
    *stack_top = 0;
    stack_top--;  // EBP
    *stack_top = 0;
    stack_top--;  // ESP (dummy)
    *stack_top = 0;
    stack_top--;  // EBX
    *stack_top = 0;
    stack_top--;  // EDX
    *stack_top = 0;
    stack_top--;  // ECX
    *stack_top = 0;
    stack_top--;  // EAX
    *stack_top = 0;
    
    // Registros de segmento
    stack_top--;  // DS
    *stack_top = 0x10;
    stack_top--;  // ES
    *stack_top = 0x10;
    stack_top--;  // FS
    *stack_top = 0x10;
    stack_top--;  // GS
    *stack_top = 0x10;
    
    proc->context.esp = (uint32_t)stack_top;
    proc->context.eip = (uint32_t)entry;
    
    // Limpiar el resto del contexto
    proc->context.edi = 0;
    proc->context.esi = 0;
    proc->context.ebp = 0;
    proc->context.ebx = 0;
    proc->context.edx = 0;
    proc->context.ecx = 0;
    proc->context.eax = 0;
    proc->context.eflags = 0x202;
    
    // Agregar a la lista
    processes[process_count] = proc;
    process_count++;
    
    vga_puts("[SCHEDULER] Added process: ", 0x1A);
    vga_puts(proc->name, 0x1A);
    vga_putc('\n', 0x1A);
}

// Ceder control al scheduler (wrapper público)
void scheduler_yield(void) {
    if (process_count <= 1) {
        return;
    }
    scheduler_yield_asm();
}

// Implementación interna del scheduler (llamada desde assembly)
void scheduler_yield_impl(void) {
    uint32_t next = (current_process + 1) % process_count;
    
    if (processes[next] == NULL || processes[next]->state != PROCESS_READY) {
        return;
    }
    
    process_t* current_proc = processes[current_process];
    process_t* next_proc = processes[next];
    
    current_process = next;
    
    // Usar task_switch con iretd uniforme
    task_switch(&current_proc->context.esp, next_proc->context.esp);
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
        // Marcar que se necesita cambio - el irq0_stub lo manejará
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

// Verificar si se necesita cambio de contexto (llamado desde loop principal)
void scheduler_check(void) {
    if (schedule_needed) {
        schedule_needed = 0;
        scheduler_yield();
    }
}
