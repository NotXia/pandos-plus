#ifndef PANDOS_EXCEPTIONS_H_INCLUDED
#define PANDOS_EXCEPTIONS_H_INCLUDED

#include <umps3/umps/cp0.h>
#include <pandos_types.h>

#define PREV_PROCESSOR_STATE    ((state_t *)BIOSDATAPAGE)
#define EXCEPTION_CODE          CAUSE_GET_EXCCODE(PREV_PROCESSOR_STATE->cause)

void exceptionHandler();

#endif