#include <exceptions.h>
#include <umps3/umps/libumps.h>
#include <pandos_types.h>
#include <initial.h>

#define PREV_PROCESSOR_STATE (state_t *)BIOSDATAPAGE

/**
 * @brief Crea un processo.
 * @param statep Stato del processo.
*/
static void createProcess(state_t *proc_state) {
    // Parametri
    state_t *statep = (state_t *)proc_state->reg_a1;
    int prio = proc_state->reg_a2;
    support_t *supportp = (support_t *)proc_state->reg_a3;

    pcb_t *new_proc = allocPcb();

    if (new_proc == NULL) {
        proc_state->reg_v0 = -1;
        return;
    }

    new_proc->p_s = *statep;
    new_proc->p_prio = (prio == 0 ? PROCESS_PRIO_LOW : PROCESS_PRIO_HIGH);
    new_proc->p_supportStruct = supportp;
    insertProcQ((prio == 0 ? low_readyqueue : high_readyqueue), curr_process);
    insertChild(curr_process, new_proc);

    proc_state->reg_v0 = new_proc->p_pid;
}

/**
 * @brief Gestore delle system call.
*/
static void systemcallHandler() {
    state_t *proc_state = PREV_PROCESSOR_STATE;
    
    switch (proc_state->reg_a0) {
        case(CREATEPROCESS):
            createProcess(proc_state);
            break;

        case(TERMPROCESS):
            break;

        case(PASSEREN):
            break;

        case(VERHOGEN):
            break;

        case(DOIO):
            break;

        case(GETTIME):
            break;

        case(CLOCKWAIT):
            break;

        case(GETSUPPORTPTR):
            break;

        case(GETPROCESSID):
            break;

        case(YIELD):
            break;
    }
}

/**
 * @brief Gestore delle eccezioni.
*/
void exceptionHandler() {
    int exception_code = (getCause() & GETEXECCODE)>>CAUSESHIFT;

    switch (exception_code) {
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