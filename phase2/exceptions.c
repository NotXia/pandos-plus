/*
    TODO Decidere se systemcall / interrupt vengono contate nel tempo CPU del processo
*/

#include <exceptions.h>
#include <umps3/umps/libumps.h>
#include <pandos_types.h>
#include <initial.h>
#include <asl.h>
#include <scheduler.h>
#include <umps3/umps/cp0.h>

#define PREV_PROCESSOR_STATE    ((state_t *)BIOSDATAPAGE)
#define EXCEPTION_CODE          CAUSE_GET_EXCCODE(PREV_PROCESSOR_STATE->cause)

#define SYSTEMCALL_CODE         PREV_PROCESSOR_STATE->reg_a0
#define PARAMETER1(type, name)  type name = (type)PREV_PROCESSOR_STATE->reg_a1
#define PARAMETER2(type, name)  type name = (type)PREV_PROCESSOR_STATE->reg_a2
#define PARAMETER3(type, name)  type name = (type)PREV_PROCESSOR_STATE->reg_a3
#define SYSTEMCALL_RETURN(ret)  PREV_PROCESSOR_STATE->reg_v0 = ret


/**
 * @brief System call per crea un processo.
*/
static void _createProcess() {
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
    insertProcQ(GET_READY_QUEUE(prio), new_proc);
    insertChild(curr_process, new_proc);

    SYSTEMCALL_RETURN(new_proc->p_pid);
}

/**
 * @brief Uccide un processo e tutti i suoi figli.
 * @param process Puntatore al processo da uccidere.
*/
static void _killProcess(pcb_t *process) {
    outChild(process);

    process_count--;
    if (isSoftBlocked(process)) {
        softblocked_count--;
    }

    pcb_t *child;
    while ((child = removeChild(process)) != NULL) {
        _killProcess(child);
    }

    freePcb(process);
}

/**
 * @brief System call per terminare un processo.
*/
static void _termProcess() {
    PARAMETER1(pid_t, pid);

    if (pid == 0) {
        _killProcess(curr_process);
        scheduler();
    }
    else {
        pcb_t *process_to_kill = getProcessByPid(pid);
        if (process_to_kill != NULL) {
            _killProcess(process_to_kill);
        }
    }    
}

/**
 * @brief Chiama la P su un semaforo.
 * @param sem Puntatore del semaforo.
*/
static void P(int *sem) {
    if (*sem == 0) {
        if (insertBlocked(sem, curr_process)) { PANIC(); } // Non ci sono semafori disponibili
        setProcessBlocked(curr_process, PREV_PROCESSOR_STATE);
        scheduler();
    }
    else {
        *sem = 0;
    }
}

/**
 * @brief Chiama la V su un semaforo.
 * @param sem Puntatore del semaforo.
*/
static void V(int *sem) {
    if (*sem == 0) {
        pcb_t *ready_proc = removeBlocked(sem);

        if (ready_proc == NULL) {
            *sem = 1;
        }
        else { // Sblocca un processo bloccato sullo stesso semaforo
            setProcessReady(ready_proc);
        }
    }
    else {
        // TODO La V è bloccante
        PANIC();
    }
}

/**
 * @brief System call per chiamare una P su un semaforo.
*/
static void _passeren() {
    PARAMETER1(int *, sem);
    P(sem);
}

/**
 * @brief System call per chiamare una V su un semaforo.
*/
static void _verhogen() {
    PARAMETER1(int *, sem);
    V(sem);
}

/**
 * @brief System call per inizializzare un'operazione di I/O.
*/
static void _doIO() {
    PARAMETER1(int *, command_address);
    PARAMETER2(int, command_value);

    *command_address = command_value;
    
    int *dev_semaphore = getIODeviceSemaphore((int)command_address);
    P(dev_semaphore);
}

/**
 * @brief System call che restituisce il tempo di CPU del processo.
*/
static void _getCPUTime() {
    SYSTEMCALL_RETURN(curr_process->p_time + timeDiff(process_start_time));
}

/**
 * @brief System call per bloccare il processo in attesa dell'interval timer.
*/
static void _clockWait() {
    P(semaphore_it);
}

/**
 * @brief System call che restituisce il puntatore alla struttura di supporto del processo.
*/
static void _getSupportPtr() {
    SYSTEMCALL_RETURN(curr_process->p_supportStruct);
}

/**
 * @brief System call che restituisce il pid del processo o del genitore.
*/
static void _getProcessId() {
    PARAMETER1(int, parent);

    if (parent == 0) {
        SYSTEMCALL_RETURN(curr_process->p_pid);
    }
    else {
        if (curr_process->p_parent == NULL) {
            SYSTEMCALL_RETURN(0);
        }
        else {
            SYSTEMCALL_RETURN(curr_process->p_parent->p_pid);
        }
    }
}

/**
 * @brief System call per rilasciare la CPU e tornare ready.
*/
static void _yield() {
    curr_process->p_time += timeDiff(process_start_time); // Aggiorna tempo CPU
    setProcessReady(curr_process);
    process_to_skip = curr_process;
}

/**
 * @brief Genera una trap.
*/
static void _generateTrap() {
    PREV_PROCESSOR_STATE->cause = (PREV_PROCESSOR_STATE->cause & 0xFFFFFF83) | 0x28; // Reserved instruction
    // _passUpOrDieHandler(GENERALEXCEPT);
}

/**
 * @brief Gestore delle system call.
*/
static void systemcallHandler() {
    // Controllo permessi (kernel mode)
    if ((SYSTEMCALL_CODE < 0) && ((PREV_PROCESSOR_STATE->status & KUC_BIT) != 0)) {
        _generateTrap();
    }

    PREV_PROCESSOR_STATE->pc_epc += WORDLEN;
    // curr_process->p_s.pc_epc += 4;

    switch (SYSTEMCALL_CODE) {
        case(CREATEPROCESS): _createProcess(); break;
        case(TERMPROCESS):   _termProcess();   break;
        case(PASSEREN):      _passeren();      break;
        case(VERHOGEN):      _verhogen();      break;
        case(DOIO):          _doIO();          break;
        case(GETTIME):       _getCPUTime();    break;
        case(CLOCKWAIT):     _clockWait();     break;
        case(GETSUPPORTPTR): _getSupportPtr(); break;
        case(GETPROCESSID):  _getProcessId();  break;
        case(YIELD):         _yield();         break;

        default:
            _generateTrap();
            break;
    }

    LDST(PREV_PROCESSOR_STATE);
}

/**
 * @brief Gestore del pass up or die.
*/
static void _passUpOrDieHandler(int index) {
    if (curr_process->p_supportStruct == NULL) {
        _killProcess(curr_process);
        scheduler();
    }
    else {
        curr_process->p_supportStruct->sup_exceptState[index] = *PREV_PROCESSOR_STATE;
        context_t ctx = curr_process->p_supportStruct->sup_exceptContext[index];
        LDCXT(ctx.stackPtr, ctx.status, ctx.pc);
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
            _passUpOrDieHandler(PGFAULTEXCEPT);
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
            _passUpOrDieHandler(GENERALEXCEPT);
            break;

        // Systemcall
        case(8):
            systemcallHandler();
            break;
    }
}