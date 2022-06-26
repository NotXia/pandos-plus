#include <sysSupport.h>
#include <pandos_types.h>
#include <initial.h> // TODO Togliere in futuro

#define SYSTEMCALL_CODE         ((int)PREV_PROCESSOR_STATE->reg_a0)
#define PARAMETER1(type, name)  type name = (type)PREV_PROCESSOR_STATE->reg_a1
#define PARAMETER2(type, name)  type name = (type)PREV_PROCESSOR_STATE->reg_a2
#define PARAMETER3(type, name)  type name = (type)PREV_PROCESSOR_STATE->reg_a3
#define SYSTEMCALL_RETURN(ret)  PREV_PROCESSOR_STATE->reg_v0 = ret

#define DEV_READY               1
#define TERMINAL_STATUS(status) (status & 0b11111111)
#define CHAR_RECEIVED           5
#define CHAR_TRANSMITTED        5

static int printer_sem[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
static int terminal_sem[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };

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
 * @param device_number Numero del device.
*/
static void _writePrinter(int device_number) {
    PARAMETER1(char*, string);
    PARAMETER2(int, length);
    dtpreg_t *dev_reg = (dtpreg_t *)DEV_REG_ADDR(6, device_number);
    int sent = 0;

    if (length < 0 || length > 128 || (memaddr)string < KUSEG) { _terminate(); }

    SYSCALL(PASSEREN, &printer_sem[device_number], NULL, NULL);
    for (int i=0; i<length; i++) {
        dev_reg->data0 = *string;
        int status = SYSCALL(DOIO, &dev_reg->command, PRINTERWRITE, NULL);
        if (status != DEV_READY) { SYSTEMCALL_RETURN(-status); }
        sent++;
        string++;
    }
    SYSCALL(VERHOGEN, &printer_sem[device_number], NULL, NULL);

    SYSTEMCALL_RETURN(sent);
}

/**
 * @brief System call per scrivere su terminale.
 * @param device_number Numero del device.
*/
static void _writeTerminal(int device_number) {
    PARAMETER1(char*, string);
    PARAMETER2(int, length);
    termreg_t *dev_reg = (termreg_t *)DEV_REG_ADDR(7, device_number);
    int sent = 0;

    if (length < 0 || length > 128 || (memaddr)string < KUSEG) { _terminate(); }

    SYSCALL(PASSEREN, &terminal_sem[device_number], NULL, NULL);
    for (int i = 0; i<length; i++) {
        int status = SYSCALL(DOIO, &dev_reg->transm_command, (TERMINALWRITE + (*string << 8)), NULL);
        if (TERMINAL_STATUS(status) != CHAR_TRANSMITTED) { SYSTEMCALL_RETURN(-status); }
        sent++;
        string++;
    }
    SYSCALL(VERHOGEN, &terminal_sem[device_number], NULL, NULL);

    SYSTEMCALL_RETURN(sent);
}

/**
 * @brief System call per leggere da terminale.
 * @param device_number Numero del device.
*/
static void _readTerminal(int device_number) {
    PARAMETER1(int*, buffer);
    termreg_t *dev_reg = (termreg_t *)DEV_REG_ADDR(7, device_number);
    int received = 0;
    char read;

    if ((memaddr)buffer < KUSEG) { _terminate(); }

    SYSCALL(PASSEREN, &terminal_sem[device_number], NULL, NULL);
    while (1) {
        int status = SYSCALL(DOIO, &dev_reg->recv_command, TERMINALREAD, NULL);
        if (TERMINAL_STATUS(status) != CHAR_RECEIVED) { SYSTEMCALL_RETURN(-status); }
        read = status >> 8;
        if (read == '\n') { break; }
        buffer[received] = read;
        received++;
    }
    SYSCALL(VERHOGEN, &terminal_sem[device_number], NULL, NULL);

    SYSTEMCALL_RETURN(received);
}

/**
 * @brief Gestore delle system call.
*/
static void _systemcallHandler(support_t *support_structure) {
    switch (SYSTEMCALL_CODE) {
        case GETTOD:         _getTOD();        break;
        case TERMINATE:      _terminate();     break;
        case WRITEPRINTER:   _writePrinter(support_structure->sup_asid-1);  break;
        case WRITETERMINAL:  _writeTerminal(support_structure->sup_asid-1); break;
        case READTERMINAL:   _readTerminal(support_structure->sup_asid-1);  break;
        default:
            break;
    }

    LDST(&support_structure->sup_exceptState[GENERALEXCEPT]);
}

/**
 * @brief Gestore delle general exception.
*/
void generalExceptionHandler() {
    support_t *support_structure = (support_t *)SYSCALL(GETSUPPORTPTR, NULL, NULL, NULL);

    switch (CAUSE_GET_EXCCODE(support_structure->sup_exceptState[GENERALEXCEPT].cause)) {
        case 8: // System call
            _systemcallHandler(support_structure);
            break;
    }
}