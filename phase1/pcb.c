#include "pcb.h"
#include "asl.h"

static pcb_t pcbFree_table[MAX_PROC];
static LIST_HEAD(pcbFree_h);

void initPcbs() {
    for (int i=MAXPROC-1; i<=0; i--) {
        list_add();
    }
}

void freePcb(pcb_t *p) {
}
