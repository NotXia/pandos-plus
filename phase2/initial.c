#include <initial.h>
#include <asl.h>
#include <pandos_const.h>
#include <pandos_types.h>
#include <exceptions.h>
#include <interrupts.h>
#include <scheduler.h>


/**
 * @brief Inizializza il pass up vector
*/
static void _initPassUpVector() {
    passupvector_t *pass_up_vector = (passupvector_t *)PASSUPVECTOR;
    pass_up_vector->tlb_refill_handler = (memaddr)uTLB_RefillHandler;
    pass_up_vector->tlb_refill_stackPtr = KERNELSTACK;
    pass_up_vector->exception_handler = (memaddr)exceptionHandler;
    pass_up_vector->exception_stackPtr = KERNELSTACK;
}

/**
 * @brief Inizializza i semafori
*/
static void _initDeviceSemaphores() {
    semaphore_it = 0;
    for (int i=0; i<TOTAL_IO_DEVICES; i++) {
        semaphore_devices[i] = 0;
    }
}

/**
 * @brief Crea il primo processo
 * @return Puntatore al PCB del processo creato
*/
static pcb_t *_createFirstProcess() {
    pcb_t *first_proc = allocPcb();

    first_proc->p_prio = PROCESS_PRIO_LOW;
    RAMTOP(first_proc->p_s.reg_sp);
    // Interrupt abilitati + PLT abilitato + Kernel mode
    first_proc->p_s.status = ALLOFF | IMON | IEPON | TEBITON;
    first_proc->p_s.s_t9 = first_proc->p_s.pc_epc = (memaddr)test;

    return first_proc;
}

/**
 * @brief Indica se un processo è soft-blocked (quindi bloccato su un qualunque device).
 * @param p Puntatore al PCB del processo da controllare.
 * @return TRUE se è soft-blocked, FALSE altrimenti.
*/
int isSoftBlocked(pcb_t *p) {
    if (p->p_semAdd == NULL) { return FALSE; }

    if (p->p_semAdd == semaphore_it) { return TRUE; }
    for (int i=0; i<TOTAL_IO_DEVICES; i++) {
        if (p->p_semAdd == semaphore_devices[i]) { return TRUE; }
    }

    return FALSE;
}

/**
 * @brief Restituisce il semaforo associato ad un dispositivo di I/O ricercato per indirizzo del campo command nel device register.
 * @param address indirizzo del campo command nel device register interessato.
 * @return Puntatore al semaforo.
*/
int *getIODeviceSemaphore(int command_address) {
    /* A partire dall'indirizzo del campo command, si calcola l'indirizzo dell'inizio del device register da cui si ricava l'indice del semaforo */
    int sem_index;
    int dev_register_address;
    int offset = 0;

    if (command_address >= TERM0ADDR) { // Terminale
        // Per i terminali i due registri command distano 2 word e quindi la loro distanza è di 0x8 (0b1000)
        // Quindi si possono differenziare tra loro valutando il valore del 4° bit
        if ((command_address & 0b1000) == 0b1000) { // Ricezione
            dev_register_address = command_address - 0x4;
        } 
        else { // Trasmissione
            dev_register_address = command_address - 0xC;
            offset = 8;
        }
    }
    else { // Altri device
        dev_register_address = command_address - 0x4;
    }

    sem_index = ((dev_register_address - TERM0ADDR) / DEVREG_SIZE) + offset;
    return &semaphore_devices[sem_index];
}

/**
 * @brief Calcola la differenza tra start e il tempo attuale.
 * @param start Tempo di inizio
 * @return La differenza di tempo.
*/
cpu_t timeDiff(cpu_t start) {
    cpu_t curr_time;
    STCK(curr_time);
    return curr_time - start;
}

void main() {
    _initPassUpVector();
    initPcbs();
    initASL();
    process_count = 0;
    softblocked_count = 0;
    mkEmptyProcQ(high_readyqueue);
    mkEmptyProcQ(low_readyqueue);
    curr_process = NULL;
    _initDeviceSemaphores();
    LDIT(PSECOND);

    insertProcQ(low_readyqueue, _createFirstProcess());

    scheduler();
}