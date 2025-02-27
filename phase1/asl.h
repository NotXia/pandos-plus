#ifndef PANDOS_ASL_H_INCLUDED
#define PANDOS_ASL_H_INCLUDED

#include <pandos_types.h>

void initASL();
int insertBlocked(int *semAdd, pcb_t *p);
pcb_t *removeBlocked(int *semAdd);
pcb_t *outBlocked(pcb_t *p);
pcb_t *headBlocked(int *semAdd);

pcb_t *semV(int *sem, pcb_t *process, state_t *state);
pcb_t *semP(int *sem, pcb_t *process, state_t *state);

#endif