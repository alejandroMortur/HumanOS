# Userland y Shell en HumanOS

## Visión General

HumanOS implementa un shell interactivo en Ring 3 (modo usuario) que proporciona una interfaz de línea de comandos estilo Bash. El shell se ejecuta como un proceso separado y usa syscalls para interactuar con el kernel.

## Creación del Proceso Shell

El shell se crea como proceso Ring 3 en kernel_main:

```c
scheduler_add_user_process(user_shell_process, "UserShellRing3");
```

## Implementación del Shell

### user_shell_process()

```c
static void user_shell_process(void) {
    sys_write(1, "Type 'help' for commands list.\n\n", 32);

    char input_buf[64];
    int buf_idx = 0;

    // Historial de comandos
    #define MAX_HIST 10
    static char cmd_history[MAX_HIST][64];
    static int hist_count = 0;
    static int hist_idx = 0;

    print_shell_prompt();

    while (1) {
        char c = 0;
        int bytes = sys_read(0, &c, 1);
        if (bytes > 0) {
            // Procesar carácter...
        } else {
            sys_yield();
        }
    }
}
```

## Comandos Implementados

### help
Lista todos los comandos disponibles.

### ls
Lista archivos en el VFS.

### cat
Lee y muestra contenido de archivo.

### touch
Crea archivo vacío.

### write
Escribe texto a archivo.

### diskinfo
Muestra información del disco duro.

### sysinfo
Muestra información del CPU.

### whoami
Retorna "user".

### hostname
Retorna "humanos".

### uname
Retorna información del sistema operativo.

### ver
Retorna versión del sistema.

### clear
Limpia la pantalla.

### ps
Lista procesos activos.

### reboot
Reinicia el sistema.

### shutdown
Apaga el sistema.

### exit
Termina el proceso shell.

### run / exec
Ejecuta binario ELF desde el VFS.

## Características

### Historial de Comandos
- Últimos 10 comandos guardados
- Navegación con flechas arriba/abajo

### Autocompletado
- Tabulador para autocompletar comandos
- Tabulador para autocompletar nombres de archivos

### Prompt
```
user@humanos:~$ 
```

## Syscalls Usados

- sys_read() - Lee del teclado
- sys_write() - Escribe a pantalla
- sys_yield() - Cede control al scheduler
- sys_exit() - Termina proceso
