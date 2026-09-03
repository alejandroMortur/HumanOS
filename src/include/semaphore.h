#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdint.h>
#include "spinlock.h"

// Estructura de Semáforo Contador
typedef struct {
    volatile int count;
    spinlock_t lock;
} semaphore_t;

// Funciones de Semáforo
void sem_init(semaphore_t* sem, int value);
void sem_wait(semaphore_t* sem);
void sem_signal(semaphore_t* sem);

#endif
