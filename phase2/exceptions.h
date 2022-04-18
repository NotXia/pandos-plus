#ifndef PANDOS_EXCEPTIONS_H_INCLUDED
#define PANDOS_EXCEPTIONS_H_INCLUDED

#include <umps3/umps/cp0.h>
#include <initial.h>
#include <pandos_types.h>
#include <interrupts.h>

#define PREV_PROCESSOR_STATE    ((state_t *)BIOSDATAPAGE)

void exceptionHandler();

#endif