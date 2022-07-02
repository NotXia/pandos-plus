#ifndef PANDOS_UTILITIES_H_INCLUDED
#define PANDOS_UTILITIES_H_INCLUDED

void memcpy(void *dest, const void *src, unsigned int n);
void P(semaphore_t *sem, int asid);
void V(semaphore_t *sem);

#endif