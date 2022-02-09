#ifndef PANDOS_ASL_H_INCLUDED
#define PANDOS_ASL_H_INCLUDED

#include "pandos_types.h"

void initPcbs();
void freePcb(pcb_t *p);
pcb_t *allocPcb();
void mkEmptyProcQ(struct list_head *head);
int emptyProcQ(struct list_head *head);
void insertProcQ(struct list_head *head, pcb *p);
pcb_t headProcQ(struct list_head *head);
pcb_t *removeProcQ(struct list_head *head);
pcb_t *outProcQ(struct list_head *head, pcb_t *p);
int emptyChild(pcb_t *p);
void insertChild(pcb_t *prnt,pcb_t *p);
pcb_t *removeChild(pcb_t *p);
pcb_t *outChild(pcb_t* p);

#endif
