#include <sysSupport.h>
#include <pandos_types.h>
#include <initial.h> // TODO Togliere in futuro
#include <utilities.h>
#include <vmSupport.h>

#define SYSTEMCALL_CODE         ((int)PREV_PROCESSOR_STATE->reg_a0)
#define PARAMETER1(type, name)  type name = (type)PREV_PROCESSOR_STATE->reg_a1
#define PARAMETER2(type, name)  type name = (type)PREV_PROCESSOR_STATE->reg_a2
#define PARAMETER3(type, name)  type name = (type)PREV_PROCESSOR_STATE->reg_a3
#define SYSTEMCALL_RETURN(ret)  PREV_PROCESSOR_STATE->reg_v0 = ret
#define DEVICE_OF(asid)         (asid - 1)

#define DEV_READY               1
#define TERMINAL_STATUS(status) (status & 0b11111111)
#define CHAR_RECEIVED           5
#define CHAR_TRANSMITTED        5

static semaphore_t printer_sem[8];
static semaphore_t terminal_sem[8];

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
static void _getTOD() {
    cpu_t curr_time;
    STCK(curr_time);
    SYSTEMCALL_RETURN(curr_time);
}

/**
 * @brief System call per terminare un processo.
*/
static void _terminate() {
    SYSCALL(TERMPROCESS, 0, NULL, NULL);
}


/**
 * @brief System call per scrivere su stampante.
 * @param asid ASID del processo.
*/
static void _writePrinter(int asid) {
    PARAMETER1(char*, string);
    PARAMETER2(int, length);
    dtpreg_t *dev_reg = (dtpreg_t *)DEV_REG_ADDR(6, DEVICE_OF(asid));
    int sent = 0;

    if (length < 0 || length > 128 || (memaddr)string < KUSEG) { _terminate(); }

    P(&printer_sem[DEVICE_OF(asid)], asid);
    for (int i=0; i<length; i++) {
        dev_reg->data0 = *string;
        int status = SYSCALL(DOIO, &dev_reg->command, PRINTERWRITE, NULL);
        if (status != DEV_READY) { SYSTEMCALL_RETURN(-status); }
        sent++;
        string++;
    }
    V(&printer_sem[DEVICE_OF(asid)]);

    SYSTEMCALL_RETURN(sent);
}

/**
 * @brief System call per scrivere su terminale.
 * @param asid ASID del processo.
*/
static void _writeTerminal(int asid) {
    PARAMETER1(char*, string);
    PARAMETER2(int, length);
    termreg_t *dev_reg = (termreg_t *)DEV_REG_ADDR(7, DEVICE_OF(asid));
    int sent = 0;

    if (length < 0 || length > 128 || (memaddr)string < KUSEG) { _terminate(); }

    P(&terminal_sem[DEVICE_OF(asid)], asid);
    for (int i = 0; i<length; i++) {
        int status = SYSCALL(DOIO, &dev_reg->transm_command, (TERMINALWRITE + (*string << 8)), NULL);
        if (TERMINAL_STATUS(status) != CHAR_TRANSMITTED) { SYSTEMCALL_RETURN(-status); }
        sent++;
        string++;
    }
    V(&terminal_sem[DEVICE_OF(asid)]);

    SYSTEMCALL_RETURN(sent);
}

/**
 * @brief System call per leggere da terminale.
 * @param asid ASID del processo.
*/
static void _readTerminal(int asid) {
    PARAMETER1(int*, buffer);
    termreg_t *dev_reg = (termreg_t *)DEV_REG_ADDR(7, DEVICE_OF(asid));
    int received = 0;
    char read;

    if ((memaddr)buffer < KUSEG) { _terminate(); }
    
    P(&terminal_sem[DEVICE_OF(asid)], asid);
    while (1) {
        int status = SYSCALL(DOIO, &dev_reg->recv_command, TERMINALREAD, NULL);
        if (TERMINAL_STATUS(status) != CHAR_RECEIVED) { SYSTEMCALL_RETURN(-status); }
        read = status >> 8;
        if (read == '\n') { break; }
        buffer[received] = read;
        received++;
    }
    V(&terminal_sem[DEVICE_OF(asid)]);

    SYSTEMCALL_RETURN(received);
}

/**
 * @brief Gestore delle system call.
*/
static void _systemcallHandler(support_t *support_structure) {
    switch (SYSTEMCALL_CODE) {
        case GETTOD:         _getTOD();        break;
        case TERMINATE:      _terminate();     break;
        case WRITEPRINTER:   _writePrinter(support_structure->sup_asid);  break;
        case WRITETERMINAL:  _writeTerminal(support_structure->sup_asid); break;
        case READTERMINAL:   _readTerminal(support_structure->sup_asid);  break;
        default:
            break;
    }

    LDST(&support_structure->sup_exceptState[GENERALEXCEPT]);
}

/**
 * @brief Gestore delle general exceptions.
*/
void generalExceptionHandler() {
    support_t *support_structure = (support_t *)SYSCALL(GETSUPPORTPTR, NULL, NULL, NULL);

    switch (CAUSE_GET_EXCCODE(support_structure->sup_exceptState[GENERALEXCEPT].cause)) {
        case 8: // System call
            _systemcallHandler(support_structure);
            break;
    }
}

/**
 * @brief Gestore delle trap exceptions.
*/
void trapExceptionHandler() {
    support_t *support_structure = (support_t *)SYSCALL(GETSUPPORTPTR, NULL, NULL, NULL);
    
    for (int i = 0; i<8; i++) {
        if (printer_sem[i].user_asid == support_structure->sup_asid) { V(&printer_sem[i]); }
        if (terminal_sem[i].user_asid == support_structure->sup_asid) { V(&terminal_sem[i]); }
    }
    releaseSwapPoolSem();

    _terminate();
}
