#ifndef PANDOS_INITIAL_H_INCLUDED
#define PANDOS_INITIAL_H_INCLUDED

#include <umps3/umps/libumps.h>
#include <pcb.h>
#include <listx.h>

unsigned int process_count;
unsigned int softblocked_count;

list_head *high_readyqueue;
list_head *low_readyqueue;
#define GET_READY_QUEUE(prio) (prio == PROCESS_PRIO_LOW ? low_readyqueue : high_readyqueue)


pcb_t *curr_process;

int semaphore_it;
int semaphore_devices[48];

#define TOTAL_IO_DEVICES 48

int isSoftBlocked(pcb_t *p);
int *getIODeviceSemaphore(int address);

#endif