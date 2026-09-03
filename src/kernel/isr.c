#include "isr.h"
#include "vga.h"
#include <stdint.h>

// Mensajes descriptivos para las 32 excepciones de x86
static const char* exception_messages[32] = {
    "Division By Zero",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

// Función auxiliar para imprimir un entero hexadecimal (uint32_t)
static void print_hex(uint32_t val, uint8_t color) {
    char buf[11];
    buf[0] = '0';
    buf[1] = 'x';
    const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        buf[2 + i] = hex_chars[val & 0xF];
        val >>= 4;
    }
    buf[10] = '\0';
    vga_puts(buf, color);
}

// Función auxiliar para imprimir un número decimal
static void print_dec(uint32_t val, uint8_t color) {
    if (val == 0) {
        vga_putc('0', color);
        return;
    }
    char buf[16];
    int i = 0;
    while (val > 0 && i < 15) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    for (int j = i - 1; j >= 0; j--) {
        vga_putc(buf[j], color);
    }
}

// Manejador centralizado de excepciones ISR (0-31)
void isr_handler(registers_t* regs) {
    uint8_t panic_color = 0x4F; // Texto blanco con fondo rojo
    
    vga_puts("\n========================================\n", panic_color);
    vga_puts("          !!! KERNEL PANIC !!!          \n", panic_color);
    vga_puts("========================================\n", panic_color);
    
    vga_puts("Exception [", panic_color);
    print_dec(regs->int_no, panic_color);
    vga_puts("]: ", panic_color);
    
    if (regs->int_no < 32) {
        vga_puts(exception_messages[regs->int_no], panic_color);
    } else {
        vga_puts("Unknown Exception", panic_color);
    }
    vga_puts("\n", panic_color);
    
    vga_puts("Error Code: ", panic_color);
    print_hex(regs->err_code, panic_color);
    vga_puts("\n", panic_color);
    
    // En caso de Page Fault (int 14), obtener y mostrar el registro CR2
    if (regs->int_no == 14) {
        uint32_t faulting_address;
        __asm__ __volatile__("mov %%cr2, %0" : "=r"(faulting_address));
        vga_puts("Faulting Address (CR2): ", panic_color);
        print_hex(faulting_address, panic_color);
        vga_puts("\n", panic_color);
    }
    
    vga_puts("\n--- Register Dump ---\n", panic_color);
    vga_puts("EIP: ", panic_color); print_hex(regs->eip, panic_color);
    vga_puts("  CS: ", panic_color); print_hex(regs->cs, panic_color);
    vga_puts("  EFLAGS: ", panic_color); print_hex(regs->eflags, panic_color);
    vga_puts("\n", panic_color);
    
    vga_puts("EAX: ", panic_color); print_hex(regs->eax, panic_color);
    vga_puts("  EBX: ", panic_color); print_hex(regs->ebx, panic_color);
    vga_puts("  ECX: ", panic_color); print_hex(regs->ecx, panic_color);
    vga_puts("\n", panic_color);
    
    vga_puts("EDX: ", panic_color); print_hex(regs->edx, panic_color);
    vga_puts("  ESI: ", panic_color); print_hex(regs->esi, panic_color);
    vga_puts("  EDI: ", panic_color); print_hex(regs->edi, panic_color);
    vga_puts("\n", panic_color);
    
    vga_puts("EBP: ", panic_color); print_hex(regs->ebp, panic_color);
    vga_puts("  DS:  ", panic_color); print_hex(regs->ds, panic_color);
    vga_puts("\n========================================\n", panic_color);
    vga_puts("System Halted.\n", panic_color);
    
    // Detener completamente el procesador
    while (1) {
        __asm__ __volatile__("cli; hlt");
    }
}
