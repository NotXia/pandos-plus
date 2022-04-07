#ifndef PANDOS_INITIAL_H_INCLUDED
#define PANDOS_INITIAL_H_INCLUDED

#include <umps3/umps/libumps.h>
#include <pcb.h>
#include <listx.h>

unsigned int process_count;
unsigned int softblocked_count;

list_head *high_readyqueue;
list_head *low_readyqueue;

pcb_t *curr_process;

int semaphore_it;
int semaphore_devices[48];


#define TERM_SEM_START_INDEX 32

#endif