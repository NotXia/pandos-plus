#include "sysSupport.h"
#include <pandos_types.h>
#include <initial.h> // TODO Togliere in futuro
#include <utilities.h>
#include <vmSupport.h>
#include <initProc.h>
#include <umps3/umps/libumps.h>
#include <umps3/umps/arch.h>
#include <umps3/umps/cp0.h>

#define PROCESSOR_STATE(supp_struct)            (supp_struct->sup_exceptState[GENERALEXCEPT])
#define SYSTEMCALL_CODE(supp_struct)            ((int)PROCESSOR_STATE(supp_struct).reg_a0)
#define PARAMETER1(type, name, supp_struct)     type name = (type)PROCESSOR_STATE(supp_struct).reg_a1
#define PARAMETER2(type, name, supp_struct)     type name = (type)PROCESSOR_STATE(supp_struct).reg_a2
#define PARAMETER3(type, name, supp_struct)     type name = (type)PROCESSOR_STATE(supp_struct).reg_a3
#define SYSTEMCALL_RETURN(ret, supp_struct)     PROCESSOR_STATE(supp_struct).reg_v0 = ret
#define DEVICE_OF(asid)                         (asid - 1)

static semaphore_t printer_sem[8];
static semaphore_t terminal_sem[8];

/**
 * @brief Inizializza i semafori.
*/
void initSysStructs() {
    for (int i=0; i<8; i++) {
        printer_sem[i].val = 1;
        printer_sem[i].user_asid = NOPROC;
        terminal_sem[i].val = 1;
        terminal_sem[i].user_asid = NOPROC;
    }
}

/**
 * @brief System call per restituire il valore del TOD.
*/
static void _getTOD(support_t *support_structure) {
    cpu_t curr_time;
    STCK(curr_time);
    SYSTEMCALL_RETURN(curr_time, support_structure);
}

/**
 * @brief System call per terminare un processo.
*/
static void _terminate(support_t *support_structure) {
    freeFrame(support_structure->sup_asid);
    signalProcessTermination();
    SYSCALL(TERMPROCESS, 0, 0, 0);
}


/**
 * @brief System call per scrivere su stampante.
 * @param asid ASID del processo.
*/
static void _writePrinter(int asid, support_t *support_structure) {
    PARAMETER1(char*, string, support_structure);
    PARAMETER2(int, length, support_structure);
    dtpreg_t *dev_reg = (dtpreg_t *)DEV_REG_ADDR(6, DEVICE_OF(asid));
    int sent = 0;

    if (length < 0 || length > 128 || (memaddr)string < KUSEG) { _terminate(support_structure); }

    P(&printer_sem[DEVICE_OF(asid)], asid);
    for (int i=0; i<length; i++) {
        dev_reg->data0 = *string;
        int status = SYSCALL(DOIO, (memaddr)&dev_reg->command, PRINTERWRITE, 0);
        if (status != DEV_READY) { SYSTEMCALL_RETURN(-status, support_structure); }
        sent++;
        string++;
    }
    V(&printer_sem[DEVICE_OF(asid)]);

    SYSTEMCALL_RETURN(sent, support_structure);
}

/**
 * @brief System call per scrivere su terminale.
 * @param asid ASID del processo.
*/
static void _writeTerminal(int asid, support_t *support_structure) {
    PARAMETER1(char*, string, support_structure);
    PARAMETER2(int, length, support_structure);
    termreg_t *dev_reg = (termreg_t *)DEV_REG_ADDR(7, DEVICE_OF(asid));
    int sent = 0;

    if (length < 0 || length > 128 || (memaddr)string < KUSEG) { _terminate(support_structure); }

    P(&terminal_sem[DEVICE_OF(asid)], asid);
    for (int i=0; i<length; i++) {
        int status = SYSCALL(DOIO, (memaddr)&dev_reg->transm_command, (TERMINALWRITE + (*string << 8)), 0);
        if (TERMINAL_STATUS(status) != CHAR_TRANSMITTED) { SYSTEMCALL_RETURN(-status, support_structure); break; }
        sent++;
        string++;
    }
    V(&terminal_sem[DEVICE_OF(asid)]);

    SYSTEMCALL_RETURN(sent, support_structure);
}

/**
 * @brief System call per leggere da terminale.
 * @param asid ASID del processo.
*/
static void _readTerminal(int asid, support_t *support_structure) {
    PARAMETER1(char*, buffer, support_structure);
    termreg_t *dev_reg = (termreg_t *)DEV_REG_ADDR(7, DEVICE_OF(asid));
    int received = 0;
    char read;

    if ((memaddr)buffer < KUSEG) { _terminate(support_structure); }
    
    P(&terminal_sem[DEVICE_OF(asid)], asid);
    while (1) {
        int status = SYSCALL(DOIO, (memaddr)&dev_reg->recv_command, TERMINALREAD, 0);      
        if (TERMINAL_STATUS(status) != CHAR_RECEIVED) { SYSTEMCALL_RETURN(-status, support_structure); break; }
      
        read = status >> 8;
        buffer[received] = read;
        received++;
        
        if (read == '\n') { break; }
    }
    V(&terminal_sem[DEVICE_OF(asid)]);

    SYSTEMCALL_RETURN(received, support_structure);
}

/**
 * @brief Gestore delle system call.
*/
static void _systemcallHandler(support_t *support_structure) {
    support_structure->sup_exceptState[GENERALEXCEPT].pc_epc += WORD_SIZE;

    switch (SYSTEMCALL_CODE(support_structure)) {
        case GETTOD:         _getTOD(support_structure);        break;
        case TERMINATE:      _terminate(support_structure);     break;
        case WRITEPRINTER:   _writePrinter(support_structure->sup_asid, support_structure);  break;
        case WRITETERMINAL:  _writeTerminal(support_structure->sup_asid, support_structure); break;
        case READTERMINAL:   _readTerminal(support_structure->sup_asid, support_structure);  break;
        default:
            break;
    }

    LDST(&support_structure->sup_exceptState[GENERALEXCEPT]);
}

/**
 * @brief Gestore delle general exceptions.
*/
void generalExceptionHandler() {
    support_t *support_structure = (support_t *)SYSCALL(GETSUPPORTPTR, 0, 0, 0);

    switch (CAUSE_GET_EXCCODE(support_structure->sup_exceptState[GENERALEXCEPT].cause)) {
        case 8: // System call
            _systemcallHandler(support_structure);
            break;
        default:
            trapExceptionHandler();
            break;
    }
}

/**
 * @brief Gestore delle trap exceptions.
*/
void trapExceptionHandler() {
    support_t *support_structure = (support_t *)SYSCALL(GETSUPPORTPTR, 0, 0, 0);
    
    for (int i = 0; i<8; i++) {
        if (printer_sem[i].user_asid == support_structure->sup_asid) { V(&printer_sem[i]); }
        if (terminal_sem[i].user_asid == support_structure->sup_asid) { V(&terminal_sem[i]); }
    }
    releaseSwapPoolSem();

    _terminate(support_structure);
}
