#ifndef PANDOS_PCB_H_INCLUDED
#define PANDOS_PCB_H_INCLUDED

#include "pandos_types.h"

void initPcbs();
void freePcb(pcb_t *p);
pcb_t *allocPcb();
void mkEmptyProcQ(struct list_head *head);
int emptyProcQ(struct list_head *head);
void insertProcQ(struct list_head *head, pcb_t *p);
pcb_t *headProcQ(struct list_head *head);
pcb_t *removeProcQ(struct list_head *head);
pcb_t *outProcQ(struct list_head *head, pcb_t *p);
int emptyChild(pcb_t *p);
void insertChild(pcb_t *prnt, pcb_t *p);

pcb_t *removeChild(pcb_t *p);
pcb_t *outChild(pcb_t *p);


#define FREE_PCB_PID    -1

// Indica se il processo relativo ad un PCB è ancora attivo in un qualunque stato di esecuzione
#define IS_ALIVE(p)     (p)->p_pid != FREE_PCB_PID

pcb_t *getProcessByPid(pid_t pid);


#endif
