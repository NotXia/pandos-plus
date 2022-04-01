#ifndef PANDOS_INITIAL_H_INCLUDED
#define PANDOS_INITIAL_H_INCLUDED

#include <umps3/umps/libumps.h>
#include <pcb.h>
#include <listx.h>

unsigned int process_count;
unsigned int softblocked_count;

struct list_head *high_readyqueue;
struct list_head *low_readyqueue;

pcb_t *curr_process;

int semaphore_plt;
int semaphore_bus;
int semaphore_disk[8];
int semaphore_flashdrive[8];
int semaphore_network[8];
int semaphore_printer[8];
int semaphore_terminal[16];


#endif