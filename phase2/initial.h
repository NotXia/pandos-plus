#ifndef PANDOS_INITIAL_H_INCLUDED
#define PANDOS_INITIAL_H_INCLUDED

#include <pcb.h>
#include <asl.h>

pcb_t *curr_process;
unsigned int process_count;
unsigned int softblocked_count;
struct list_head low_readyqueue;
struct list_head high_readyqueue;
#define GET_READY_QUEUE(prio) (prio == PROCESS_PRIO_LOW ? &low_readyqueue : &high_readyqueue)

#define TOTAL_IO_DEVICES 48
int semaphore_it;
int semaphore_devices[TOTAL_IO_DEVICES];


int isSoftBlocked(pcb_t *p);
int *getIODeviceSemaphore(memaddr address);

void resetIntervalTimer();
cpu_t timerFlush();
void updateProcessCPUTime();
void startPLT();

#endif