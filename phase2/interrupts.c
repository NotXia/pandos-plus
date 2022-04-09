#include <exceptions.h>
#include <umps3/umps/arch.h>
#include <umps3/umps/libumps.h>
#include <scheduler.h>
#include <initial.h>
#include <asl.h>

#define RETURN_TO_CURRENT_PROCESS       if (curr_process != NULL) { LDST(PREV_PROCESSOR_STATE); }

/**
 * @brief Gestore del Processor Local Timer.
*/
static void _PLTHandler() {
    setTimer(TIMESLICE); // Ack PLT

    curr_process->p_s = *PREV_PROCESSOR_STATE;
    curr_process->p_time += timeDiff(process_start_time);

    setProcessReady(curr_process);
    scheduler();
}

/**
 * @brief Gestore dell'Interval Timer.
*/
static void _ITHandler() {
    LDIT(PSECOND); // Reset IT

    // Riattiva tutti i processi in attesa dell'IT (operazione di V fittizia)
    pcb_t *p;
    while ((p = headBlocked(&semaphore_it)) != NULL) {
        V(&semaphore_it);
    }

    semaphore_it = 0;

    RETURN_TO_CURRENT_PROCESS;
}

/**
 * @brief Restituisce la bitmap relativa alla linea.
 * @param line Linea di interrupt.
 * @return La bitmap della linea di interrupt.
*/
static unsigned int _getDeviceInterruptBitmap(int line) {
    devregarea_t *bus_reg_area = (devregarea_t*)BUS_REG_RAM_BASE;
    return bus_reg_area->interrupt_dev[line-3];
}

/**
 * @brief Gestore dei device di I/O.
 * @param line Linea di interrupt.
*/
static void _deviceHandler(int line) {
    unsigned int bitmap = _getDeviceInterruptBitmap(line);
    unsigned int mask = 1;
    int device_number = 0;

    // Individua il device che ha generato l'interrupt
    for (int i=0; i<N_DEV_PER_IL; i++) {
        if (bitmap & mask != 0) {
            if (line != IL_TERMINAL) {
                _nonTerminalHandler(line, device_number);
            }
            else {
                _terminalHandler(device_number);
            }
            
            RETURN_TO_CURRENT_PROCESS;

            break; // Nel caso non ci siano processi a cui ritornare il controllo
        }
        mask = mask << 1;
        device_number++;
    }
}

/**
 * @brief Gestore devices non terminali.
 * @param line Linea di interrupt.
 * @param device_number Numero del device.
*/
static void _nonTerminalHandler(int line, int device_number) {
    dtpreg_t *device_register = (dtpreg_t *)DEV_REG_ADDR(line, device_number);

    unsigned int status_code = device_register->status;
    device_register->command = ACK;

    pcb_t *ready_proc = V(getIODeviceSemaphore((int)&device_register->command));
    if (ready_proc != NULL) {
        ready_proc->p_s.reg_v0 = status_code;
    }
}

/**
 * @brief Gestore dei terminali.
 * @param device_number Numero del device.
*/
static void _terminalHandler(int device_number) {
    termreg_t *device_register = (termreg_t *)DEV_REG_ADDR(IL_TERMINAL, device_number);

    if (device_register->recv_status == 5) {
        unsigned int status_code = device_register->recv_status;
        device_register->recv_command = ACK;

        pcb_t *ready_proc = V(getIODeviceSemaphore((int)&device_register->recv_command));
        if (ready_proc != NULL) { ready_proc->p_s.reg_v0 = status_code; }
    }
    else {
        unsigned int status_code = device_register->transm_status;
        device_register->transm_command = ACK;

        pcb_t *ready_proc = V(getIODeviceSemaphore((int)&device_register->transm_command));
        if (ready_proc != NULL) { ready_proc->p_s.reg_v0 = status_code; }
    }
}

/**
 * @brief Gestore degli interrupts.
*/
void interruptHandler() {
    // Priorità: PLT > IT > Disco > Flash drive > Stampanti > Terminali (ricezione) > Terminali (trasmissione)
    unsigned int ip = PREV_PROCESSOR_STATE->cause & CAUSE_IP_MASK;

    if (ip & LOCALTIMERINT != 0) {          // Linea 1 (PLT)
        _PLTHandler();
    }
    else if (ip & TIMERINTERRUPT != 0) {    // Line 2 (IT)
        _ITHandler();
    }
    else if (ip & DISKINTERRUPT != 0) {     // Line 3
        _deviceHandler(IL_DISK);
    }
    else if (ip & FLASHINTERRUPT != 0) {    // Line 4
        _deviceHandler(IL_FLASH);
    }
    else if (ip & NETINTERRUPT != 0) {      // Line 5
        _deviceHandler(IL_ETHERNET);
    }
    else if (ip & PRINTINTERRUPT != 0) {    // Line 6
        _deviceHandler(IL_PRINTER);
    }
    else if (ip & TERMINTERRUPT != 0) {     // Line 7
        _deviceHandler(IL_TERMINAL);
    }
}
