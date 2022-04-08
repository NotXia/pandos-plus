#ifndef PANDOS_EXCEPTIONS_H_INCLUDED
#define PANDOS_EXCEPTIONS_H_INCLUDED

#include <umps3/umps/cp0.h>
#include <pandos_types.h>

#define PREV_PROCESSOR_STATE    ((state_t *)BIOSDATAPAGE)
#define EXCEPTION_CODE          CAUSE_GET_EXCCODE(PREV_PROCESSOR_STATE->cause)

#define SYSTEMCALL_CODE         PREV_PROCESSOR_STATE->reg_a0
#define PARAMETER1(type, name)  type name = (type)PREV_PROCESSOR_STATE->reg_a1
#define PARAMETER2(type, name)  type name = (type)PREV_PROCESSOR_STATE->reg_a2
#define PARAMETER3(type, name)  type name = (type)PREV_PROCESSOR_STATE->reg_a3
#define SYSTEMCALL_RETURN(ret)  PREV_PROCESSOR_STATE->reg_v0 = ret

void exceptionHandler();

#endif