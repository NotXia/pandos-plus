#ifndef PANDOS_SCHEDULER_H_INCLUDED
#define PANDOS_SCHEDULER_H_INCLUDED

#include <umps3/umps/libumps.h>

pcb_t *process_to_skip;

void scheduler();

void setProcessBlocked(pcb_t *p, state_t *state);
void setProcessReady(pcb_t *p);

#endif