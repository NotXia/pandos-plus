#include <exceptions.h>
#include <umps3/umps/libumps.h>
#include <pandos_types.h>
#include <initial.h>

#define EXCEPTION_CODE          (getCause() & GETEXECCODE)>>CAUSESHIFT
#define PREV_PROCESSOR_STATE    ((state_t *)BIOSDATAPAGE)

#define SYSTEMCALL_CODE         PREV_PROCESSOR_STATE->reg_a0
#define PARAMETER1(type, name)  type name = (type)PREV_PROCESSOR_STATE->reg_a1
#define PARAMETER2(type, name)  type name = (type)PREV_PROCESSOR_STATE->reg_a2
#define PARAMETER3(type, name)  type name = (type)PREV_PROCESSOR_STATE->reg_a3
#define SYSTEMCALL_RETURN(ret)  PREV_PROCESSOR_STATE->reg_v0 = ret

/**
 * @brief System call per crea un processo.
*/
static void createProcess() {
    PARAMETER1(state_t *, statep);
    PARAMETER2(int, prio);
    PARAMETER3(support_t *, supportp);

    pcb_t *new_proc = allocPcb();

    if (new_proc == NULL) {
        SYSTEMCALL_RETURN(-1);
        return;
    }

    new_proc->p_s = *statep;
    new_proc->p_prio = (prio == 0 ? PROCESS_PRIO_LOW : PROCESS_PRIO_HIGH);
    new_proc->p_supportStruct = supportp;
    insertProcQ((prio == 0 ? low_readyqueue : high_readyqueue), curr_process);
    insertChild(curr_process, new_proc);

    SYSTEMCALL_RETURN(new_proc->p_pid);
}

/**
 * @brief System call per terminare un processo.
*/
void termProcess() {
    PARAMETER1(int, pid);

}

/**
 * @brief System call per chiamare una P su un semaforo.
*/
void passeren() {
    PARAMETER1(int *, sem);

}

/**
 * @brief System call per chiamare una V su un semaforo.
*/
void verhogen() {
    PARAMETER1(int *, sem);

}

/**
 * @brief System call per inizializzare un'operazione di I/O.
*/
void doIO() {
    PARAMETER1(int *, command_address);
    PARAMETER2(int, command_value);

}

/**
 * @brief System call che restituisce il tempo di CPU del processo.
*/
void getCPUTime() {

}

/**
 * @brief System call per bloccare il processo in attesa dell'interval timer.
*/
void clockWait() {

}

/**
 * @brief System call che restituisce il puntatore alla struttura di supporto del processo.
*/
void getSupportPtr() {

}

/**
 * @brief System call che restituisce il pid del processo o del genitore.
*/
void getProcessId() {
    PARAMETER1(int, parent);

}

/**
 * @brief System call per rilasciare la CPU e tornare ready.
*/
void yield() {

}


/**
 * @brief Gestore delle system call.
*/
static void systemcallHandler() {
    switch (SYSTEMCALL_CODE) {
        case(CREATEPROCESS): createProcess(); break;
        case(TERMPROCESS):   termProcess();   break;
        case(PASSEREN):      passeren();      break;
        case(VERHOGEN):      verhogen();      break;
        case(DOIO):          doIO();          break;
        case(GETTIME):       getCPUTime();    break;
        case(CLOCKWAIT):     clockWait();     break;
        case(GETSUPPORTPTR): getSupportPtr(); break;
        case(GETPROCESSID):  getProcessId();  break;
        case(YIELD):         yield();         break;

        default:
            break;
    }
}

/**
 * @brief Gestore delle eccezioni.
*/
void exceptionHandler() {
    switch (EXCEPTION_CODE) {
        // Interrupts
        case(0):
            break;

        // TLB exceptions
        case(1): 
        case(2):
        case(3):
            break;

        // Program traps
        case(4):
        case(5):
        case(6):
        case(7):
        case(9):
        case(10):
        case(11):
        case(12):
            break;

        // Systemcall
        case(8):
            systemcallHandler();
            break;
    }
}