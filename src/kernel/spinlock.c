#include "spinlock.h"
#include <stdint.h>

// Inicializar Spinlock
void spinlock_init(spinlock_t* lock) {
    if (lock != ((void*)0)) {
        lock->locked = 0;
    }
}

// Adquirir Spinlock (espera activa atómica con xchg/test_and_set)
void spinlock_lock(spinlock_t* lock) {
    if (lock == ((void*)0)) return;
    
    while (__sync_lock_test_and_set(&lock->locked, 1)) {
        __asm__ __volatile__("pause");
    }
}

// Liberar Spinlock
void spinlock_unlock(spinlock_t* lock) {
    if (lock == ((void*)0)) return;
    
    __sync_lock_release(&lock->locked);
}
