#include "libc.h"
#include <stdarg.h>

// Llamadas al sistema en ensamblador inline (int 0x80)
static inline int syscall1(int num, int arg1) {
    int ret;
    __asm__ __volatile__("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1));
    return ret;
}

static inline int syscall3(int num, int arg1, int arg2, int arg3) {
    int ret;
    __asm__ __volatile__("int $0x80" : "=a"(ret) : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3));
    return ret;
}

// ----------------------------------------------------
// Syscall Wrappers
// ----------------------------------------------------

uint32_t getpid(void) {
    return (uint32_t)syscall1(1, 0); // SYS_GETPID = 1
}

void yield(void) {
    syscall1(2, 0); // SYS_YIELD = 2
}

void exit(int status) {
    syscall1(3, status); // SYS_EXIT = 3
    while (1);
}

void* malloc(uint32_t size) {
    return (void*)syscall1(4, (int)size); // SYS_MALLOC = 4
}

void free(void* ptr) {
    (void)ptr; // Heap estático por bitmap
}

static int sys_read(int fd, void* buf, uint32_t count) {
    return syscall3(5, fd, (int)buf, (int)count); // SYS_READ = 5
}

static int sys_write(int fd, const void* buf, uint32_t count) {
    return syscall3(6, fd, (int)buf, (int)count); // SYS_WRITE = 6
}

// ----------------------------------------------------
// Manipulación de Cadenas & Memoria
// ----------------------------------------------------

uint32_t strlen(const char* str) {
    uint32_t len = 0;
    while (str[len] != '\0') len++;
    return len;
}

char* strcpy(char* dest, const char* src) {
    uint32_t i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

int strcmp(const char* s1, const char* s2) {
    uint32_t i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i]) return (s1[i] < s2[i]) ? -1 : 1;
        i++;
    }
    if (s1[i] == s2[i]) return 0;
    return (s1[i] < s2[i]) ? -1 : 1;
}

char* strcat(char* dest, const char* src) {
    uint32_t dlen = strlen(dest);
    uint32_t i = 0;
    while (src[i] != '\0') {
        dest[dlen + i] = src[i];
        i++;
    }
    dest[dlen + i] = '\0';
    return dest;
}

int atoi(const char* str) {
    int res = 0;
    int sign = 1;
    int i = 0;
    while (str[i] == ' ') i++;
    if (str[i] == '-') { sign = -1; i++; }
    else if (str[i] == '+') i++;
    while (str[i] >= '0' && str[i] <= '9') {
        res = res * 10 + (str[i] - '0');
        i++;
    }
    return res * sign;
}

char* itoa(int value, char* str, int base) {
    if (base < 2 || base > 36) { str[0] = '\0'; return str; }
    int i = 0;
    int is_neg = 0;
    unsigned int num;
    if (value < 0 && base == 10) { is_neg = 1; num = -value; }
    else { num = (unsigned int)value; }
    if (num == 0) { str[i++] = '0'; str[i] = '\0'; return str; }
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num /= base;
    }
    if (is_neg) str[i++] = '-';
    str[i] = '\0';
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++; end--;
    }
    return str;
}

void* memset(void* ptr, int value, uint32_t num) {
    unsigned char* p = (unsigned char*)ptr;
    for (uint32_t i = 0; i < num; i++) p[i] = (unsigned char)value;
    return ptr;
}

void* memcpy(void* destination, const void* source, uint32_t num) {
    unsigned char* d = (unsigned char*)destination;
    const unsigned char* s = (const unsigned char*)source;
    for (uint32_t i = 0; i < num; i++) d[i] = s[i];
    return destination;
}

// ----------------------------------------------------
// Entrada / Salida (I/O)
// ----------------------------------------------------

int putchar(char c) {
    return sys_write(1, &c, 1);
}

int puts(const char* str) {
    int len = sys_write(1, str, strlen(str));
    putchar('\n');
    return len + 1;
}

char getchar(void) {
    char c = 0;
    while (sys_read(0, &c, 1) <= 0) {
        yield();
    }
    return c;
}

int readline(char* buffer, int max_len) {
    int idx = 0;
    while (idx < max_len - 1) {
        char c = getchar();
        if (c == '\r' || c == '\n') {
            putchar('\n');
            break;
        } else if (c == '\b') {
            if (idx > 0) {
                idx--;
                sys_write(1, "\b \b", 3);
            }
        } else {
            buffer[idx++] = c;
            putchar(c);
        }
    }
    buffer[idx] = '\0';
    return idx;
}

int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buf[32];
    int count = 0;

    for (int i = 0; format[i] != '\0'; i++) {
        if (format[i] == '%') {
            i++;
            if (format[i] == 's') {
                const char* s = va_arg(args, const char*);
                if (s == NULL) s = "(null)";
                count += sys_write(1, s, strlen(s));
            } else if (format[i] == 'd') {
                int val = va_arg(args, int);
                itoa(val, buf, 10);
                count += sys_write(1, buf, strlen(buf));
            } else if (format[i] == 'x') {
                int val = va_arg(args, int);
                itoa(val, buf, 16);
                count += sys_write(1, buf, strlen(buf));
            } else if (format[i] == 'c') {
                char c = (char)va_arg(args, int);
                count += sys_write(1, &c, 1);
            } else if (format[i] == '%') {
                count += sys_write(1, "%", 1);
            }
        } else {
            count += sys_write(1, &format[i], 1);
        }
    }

    va_end(args);
    return count;
}

void sleep(uint32_t ms) {
    uint32_t ticks = ms / 10;
    if (ticks == 0) ticks = 1;
    for (uint32_t i = 0; i < ticks * 500000; i++) {
        __asm__ __volatile__("pause");
    }
}
