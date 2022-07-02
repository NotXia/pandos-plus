/*
    Fonte: https://stackoverflow.com/questions/66809657/undefined-reference-to-memcpy-error-caused-by-ld
*/

#include "utilities.h"
#include <pandos_types.h>

void memcpy(void *dest, const void *src, unsigned int n) {
    for (unsigned int i=0; i<n; i++) {
        ((char *)dest)[i] = ((char *)src)[i];
    }
}

/**
 * @brief Wrapper per P sui semafori dei device.
 * @param sem Struttura del semaforo su cui fare la P.
 * @param asid ASID del processo.
*/
void P(semaphore_t *sem, int asid) {
    SYSCALL(PASSEREN, &sem->val, NULL, NULL);
    sem->user_asid = asid;
}

/**
 * @brief Wrapper per V sui semafori dei device.
 * @param sem Struttura del semaforo su cui fare la V.
*/
void V(semaphore_t *sem) {
    sem->user_asid = NOPROC;
    SYSCALL(VERHOGEN, &sem->val, NULL, NULL);
}