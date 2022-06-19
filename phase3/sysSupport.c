#include <sysSupport.h>
#include <pandos_types.h>
#include <initial.h> // TODO Togliere in futuro

#define SYSTEMCALL_CODE         ((int)PREV_PROCESSOR_STATE->reg_a0)
#define PARAMETER1(type, name)  type name = (type)PREV_PROCESSOR_STATE->reg_a1
#define PARAMETER2(type, name)  type name = (type)PREV_PROCESSOR_STATE->reg_a2
#define PARAMETER3(type, name)  type name = (type)PREV_PROCESSOR_STATE->reg_a3
#define SYSTEMCALL_RETURN(ret)  PREV_PROCESSOR_STATE->reg_v0 = ret

static int printer_sem[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };

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

    if (length < 0 || length > 128 || (int)string < KUSEG) { _terminate(); }

    SYSCALL(PASSEREN, &printer_sem[device_number], NULL, NULL);

    for (int i=0; i<length; i++) {
        dev_reg->data0 = *string;
        int status = SYSCALL(DOIO, &dev_reg->command, PRINTERWRITE, NULL);
        if (status != 1) { // Device ready
            PANIC();
        }
        string++;
    }

    SYSCALL(VERHOGEN, &printer_sem[device_number], NULL, NULL);
}

/**
 * @brief 
*/
static void _writeTerminal() {

}

/**
 * @brief
*/
static void _readTerminal() {
    
}

/**
 * @brief Gestore delle system call.
*/
static void _systemcallHandler(support_t *support_structure) {
    switch (SYSTEMCALL_CODE) {
        case GETTOD:         _getTOD();        break;
        case TERMINATE:      _terminate();     break;
        case WRITEPRINTER:   _writePrinter(support_structure->sup_asid-1);  break;
        case WRITETERMINAL:  _writeTerminal(); break;
        case READTERMINAL:   _readTerminal();  break;
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