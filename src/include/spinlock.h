#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <stdint.h>

// Estructura de Cerrojo Atómico (Spinlock)
typedef struct {
    volatile uint32_t locked;
} spinlock_t;

// Funciones de Spinlock
void spinlock_init(spinlock_t* lock);
void spinlock_lock(spinlock_t* lock);
void spinlock_unlock(spinlock_t* lock);

#endif
