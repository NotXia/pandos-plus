#ifndef PANDOS_INITIAL_H_INCLUDED
#define PANDOS_INITIAL_H_INCLUDED

#include <pcb.h>
#include <asl.h>

extern pcb_t *curr_process;
extern unsigned int process_count;
extern unsigned int softblocked_count;
extern struct list_head low_readyqueue;
extern struct list_head high_readyqueue;
#define GET_READY_QUEUE(prio) (prio == PROCESS_PRIO_LOW ? &low_readyqueue : &high_readyqueue)

#define TOTAL_IO_DEVICES 48
extern int semaphore_it;
extern int semaphore_devices[TOTAL_IO_DEVICES];


int isSoftBlocked(pcb_t *p);
int *getIODeviceSemaphore(memaddr address);

void resetIntervalTimer();
cpu_t timerFlush();
void updateProcessCPUTime();
void startPLT();

#define PREV_PROCESSOR_STATE    ((state_t *)BIOSDATAPAGE)

#endif