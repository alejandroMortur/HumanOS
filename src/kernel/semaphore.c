#include "semaphore.h"
#include "scheduler.h"

// Inicializar Semáforo
void sem_init(semaphore_t* sem, int value) {
    if (sem != ((void*)0)) {
        sem->count = value;
        spinlock_init(&sem->lock);
    }
}

// Operación P / Wait (decrementar / esperar si no hay recurso)
void sem_wait(semaphore_t* sem) {
    if (sem == ((void*)0)) return;
    
    while (1) {
        spinlock_lock(&sem->lock);
        if (sem->count > 0) {
            sem->count--;
            spinlock_unlock(&sem->lock);
            break;
        }
        spinlock_unlock(&sem->lock);
        scheduler_yield();
    }
}

// Operación V / Signal (incrementar / notificar recurso disponible)
void sem_signal(semaphore_t* sem) {
    if (sem == ((void*)0)) return;
    
    spinlock_lock(&sem->lock);
    sem->count++;
    spinlock_unlock(&sem->lock);
}
