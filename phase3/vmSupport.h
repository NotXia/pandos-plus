#ifndef PANDOS_VMSUPPORT_H_INCLUDED
#define PANDOS_VMSUPPORT_H_INCLUDED

#include <pandos_types.h>

void initSwapStructs();
void TLBRefillHandler();
void TLBExceptionHandler();

void releaseSwapPoolSem();

void freeFrame(int asid);
void initPageTable(int asid, pteEntry_t *page_table, memaddr tmp_frame);

#endif