#ifndef PANDOS_SCHEDULER_H_INCLUDED
#define PANDOS_SCHEDULER_H_INCLUDED

#include <pcb.h>

extern pcb_t *process_to_skip; // Per gestire i processi che chiamano yield

void scheduler();

void setProcessBlocked(pcb_t *p, state_t *state);
void setProcessReady(pcb_t *p);

#endif