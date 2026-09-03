#ifndef LIBC_H
#define LIBC_H

#include <stdint.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

// Funciones de Entrada / Salida
int printf(const char* format, ...);
int puts(const char* str);
int putchar(char c);
char getchar(void);
int readline(char* buffer, int max_len);

// Manipulación de Cadenas
uint32_t strlen(const char* str);
char* strcpy(char* dest, const char* src);
int strcmp(const char* s1, const char* s2);
char* strcat(char* dest, const char* src);
int atoi(const char* str);
char* itoa(int value, char* str, int base);

// Gestión de Memoria
void* memset(void* ptr, int value, uint32_t num);
void* memcpy(void* destination, const void* source, uint32_t num);
void* malloc(uint32_t size);
void free(void* ptr);

// Control de Procesos y Sistema
uint32_t getpid(void);
void yield(void);
void exit(int status);
void sleep(uint32_t ms);

#endif
