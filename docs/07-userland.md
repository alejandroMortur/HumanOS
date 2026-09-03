# Userland and Shell in HumanOS

## Overview

HumanOS implements an interactive shell in Ring 3 (user mode) that provides a Bash-style command line interface. The shell runs as a separate process and uses syscalls to interact with the kernel.

## Shell Process Creation

The shell is created as a Ring 3 process in kernel_main:

```c
scheduler_add_user_process(user_shell_process, "UserShellRing3");
```

## Shell Implementation

### user_shell_process()

```c
static void user_shell_process(void) {
    sys_write(1, "Type 'help' for commands list.\n\n", 32);

    char input_buf[64];
    int buf_idx = 0;

    // Command history
    #define MAX_HIST 10
    static char cmd_history[MAX_HIST][64];
    static int hist_count = 0;
    static int hist_idx = 0;

    print_shell_prompt();

    while (1) {
        char c = 0;
        int bytes = sys_read(0, &c, 1);
        if (bytes > 0) {
            // Process character...
        } else {
            sys_yield();
        }
    }
}
```

## Implemented Commands

### help
Lists all available commands.

### ls
Lists files in VFS.

### cat
Reads and displays file contents.

### touch
Creates empty file.

### write
Writes text to file.

### diskinfo
Displays hard disk information.

### sysinfo
Displays CPU information.

### whoami
Returns "user".

### hostname
Returns "humanos".

### uname
Returns operating system information.

### ver
Returns system version.

### clear
Clears the screen.

### ps
Lists active processes.

### reboot
Reboots the system.

### shutdown
Shuts down the system.

### exit
Terminates the shell process.

### run / exec
Executes ELF binary from VFS.

## Features

### Command History
- Last 10 commands saved
- Navigation with up/down arrows

### Autocomplete
- Tab to autocomplete commands
- Tab to autocomplete file names

### Prompt
```
user@humanos:~$ 
```

## Used Syscalls

- sys_read() - Read from keyboard
- sys_write() - Write to screen
- sys_yield() - Yield control to scheduler
- sys_exit() - Terminate process
