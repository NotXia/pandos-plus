#include <exceptions.h>
#include <umps3/umps/arch.h>
#include <umps3/umps/libumps.h>
#include <initial.h>
#include <scheduler.h>
#include <utilities.h>

/**
 * @brief Gestisce l'uscita dall'interrupt handler.
*/
static void _interruptHandlerExit() {
    // Riattiva PLT
    setSTATUS(getSTATUS() | TEBITON);
    timerFlush();

    if (curr_process != NULL) { 
        LDST(PREV_PROCESSOR_STATE); 
    }
    else {
        scheduler(); 
    }
}

/**
 * @brief Gestore del Processor Local Timer.
*/
static void _PLTHandler() {
    setTIMER(TIMESLICE); // Ack PLT

    curr_process->p_s = *PREV_PROCESSOR_STATE;
    curr_process->p_time += timerFlush();

    setProcessReady(curr_process);
    curr_process = NULL;
    scheduler();
}

/**
 * @brief Gestore dell'Interval Timer.
*/
static void _ITHandler() {
    // Riattiva tutti i processi in attesa dell'IT
    while (headBlocked(&semaphore_it) != NULL) {
        // V(&semaphore_it);
        pcb_t *ready_proc = removeBlocked(&semaphore_it);
        setProcessReady(ready_proc);

        softblocked_count--;
    }

    semaphore_it = 0;

    // Per evitare che i ritardi si accumulino
    cpu_t curr_time;
    STCK(curr_time);
    LDIT(PSECOND - (curr_time % PSECOND)); // Reset IT
    _interruptHandlerExit();
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
 * @brief Gestisce il valore di ritorno di un device.
 * @param status Puntatore al campo status del device.
 * @param command Puntatore al campo command del device.
*/
static void _deviceInterruptReturn(unsigned int *status, unsigned int *command) {
    unsigned int status_code = *status;
    *command = ACK;

    pcb_t *ready_proc = V(getIODeviceSemaphore((memaddr)command));
    if (IS_ALIVE(ready_proc)) {
        softblocked_count--;
        ready_proc->p_s.reg_v0 = status_code;
    }
}

/**
 * @brief Gestore devices non terminali.
 * @param line Linea di interrupt.
 * @param device_number Numero del device.
*/
static void _nonTerminalHandler(int line, int device_number) {
    dtpreg_t *device_register = (dtpreg_t *)DEV_REG_ADDR(line, device_number);
    _deviceInterruptReturn(&device_register->status, &device_register->command);
}

/**
 * @brief Gestore dei terminali.
 * @param device_number Numero del device.
*/
static void _terminalHandler(int device_number) {
    termreg_t *device_register = (termreg_t *)DEV_REG_ADDR(IL_TERMINAL, device_number);

    if (device_register->recv_status == 5) {
        _deviceInterruptReturn(&device_register->recv_status, &device_register->recv_command);
    }
    else {
        _deviceInterruptReturn(&device_register->transm_status, &device_register->transm_command);
    }
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
        if ((bitmap & mask) != 0) {
            if (line != IL_TERMINAL) {
                _nonTerminalHandler(line, device_number);
            }
            else {
                _terminalHandler(device_number);
            }
            
            _interruptHandlerExit();
        }
        mask = mask << 1;
        device_number++;
    }
}

/**
 * @brief Gestore degli interrupts.
*/
void interruptHandler() {
    // Disattiva PLT
    setSTATUS(getSTATUS() & ~TEBITON);
    if (curr_process != NULL) {
        curr_process->p_time += timerFlush();
    }

    // Priorità: PLT > IT > Disco > Flash drive > Stampanti > Terminali (ricezione) > Terminali (trasmissione)
    unsigned int ip = PREV_PROCESSOR_STATE->cause & CAUSE_IP_MASK;

    if ((ip & LOCALTIMERINT) != 0) {          // Linea 1 (PLT)
        _PLTHandler();
    }
    else if ((ip & TIMERINTERRUPT) != 0) {    // Line 2 (IT)
        _ITHandler();
    }
    else if ((ip & DISKINTERRUPT) != 0) {     // Line 3
        _deviceHandler(IL_DISK);
    }
    else if ((ip & FLASHINTERRUPT) != 0) {    // Line 4
        _deviceHandler(IL_FLASH);
    }
    else if ((ip & NETINTERRUPT) != 0) {      // Line 5
        _deviceHandler(IL_ETHERNET);
    }
    else if ((ip & PRINTINTERRUPT) != 0) {    // Line 6
        _deviceHandler(IL_PRINTER);
    }
    else if ((ip & TERMINTERRUPT) != 0) {     // Line 7
        _deviceHandler(IL_TERMINAL);
    }
}
