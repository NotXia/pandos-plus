#ifndef PANDOS_VMSUPPORT_H_INCLUDED
#define PANDOS_VMSUPPORT_H_INCLUDED

void initSwapStructs();
void TLBRefillHandler();
void TLBExceptionHandler();

void releaseSwapPoolSem();

void freeFrame(int asid);

#endif