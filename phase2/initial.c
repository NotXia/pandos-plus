#include <initial.h>
#include <pandos_types.h>
#include <umps3/umps/arch.h>
#include <umps3/umps/libumps.h>
#include <scheduler.h>
#include <exceptions.h>
#include <utilities.h>

pcb_t *curr_process;
unsigned int process_count;
unsigned int softblocked_count;
struct list_head low_readyqueue;
struct list_head high_readyqueue;
int semaphore_it;
int semaphore_devices[TOTAL_IO_DEVICES];

// Funzioni fornite dal test
extern void test();
extern void TLBRefillHandler();

/**
 * @brief Indica se un processo è soft-blocked (quindi bloccato su un qualunque device).
 * @param p Puntatore al PCB del processo da controllare.
 * @return TRUE se è soft-blocked, FALSE altrimenti.
*/
int isSoftBlocked(pcb_t *p) {
    if (p->p_semAdd == NULL) { return FALSE; }

    if (p->p_semAdd == &semaphore_it) { return TRUE; }

    // Il semaforo fa riferimento ad un device di I/O
    if ((p->p_semAdd >= &semaphore_devices[0]) && (p->p_semAdd <= &semaphore_devices[TOTAL_IO_DEVICES-1])) { return TRUE; }

    return FALSE;
}

/**
 * @brief Restituisce il semaforo associato ad un dispositivo di I/O ricercato per indirizzo del campo command nel device register.
 * @param command_address indirizzo del campo command nel device register interessato.
 * @return Puntatore al semaforo di riferimento.
*/
int *getIODeviceSemaphore(memaddr command_address) {
    /* A partire dall'indirizzo del campo command, si calcola l'indirizzo dell'inizio del device register da cui si ricava l'indice del semaforo */
    int sem_index;
    memaddr dev_register_address;
    memaddr offset = 0; // Per distinguere i sotto-registri dei terminali 

    if (command_address >= TERM0ADDR) { // Terminale
        // Per i terminali i due registri command distano 2 word e quindi la loro distanza è di 0x8 (0b1000)
        // Per cui si possono differenziare valutando il valore del 4° bit
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

    sem_index = ((dev_register_address - DEV_REG_START) / DEV_REG_SIZE) + offset;
    return &semaphore_devices[sem_index];
}

/**
 * @brief Imposta l'interval timer considerando i possibili ritardi accumulati.
*/
void resetIntervalTimer() {
    // I ritardi vengono calcolati rispetti al tempo 0 del TOD
    cpu_t curr_time;
    STCK(curr_time);
    LDIT(PSECOND - (curr_time % PSECOND));
}

/**
 * @brief Calcola la differenza tra il tempo attuale e il tempo dall'ultima chiamata (similmente ad un cronometro).
 * @return La differenza di tempo.
*/
cpu_t timerFlush() {
    static cpu_t timer_start = 0;
    cpu_t curr_time, diff;

    STCK(curr_time);
    diff = curr_time - timer_start;
    
    STCK(timer_start); // Reset tempo di inizio

    return diff;
}

/**
 * @brief Aggiorna il tempo di CPU accumulato del processo corrente.
*/
void updateProcessCPUTime() {
    curr_process->p_time += timerFlush();
}

/**
 * @brief Avvia il PLT impostato correttamente con il timeslice.
*/
void startPLT() {
    setTIMER((cpu_t)TIMESLICE * (*((cpu_t *)TIMESCALEADDR)));
}


/**
 * @brief Inizializza il pass up vector.
*/
static void _initPassUpVector() {
    passupvector_t *pass_up_vector = (passupvector_t *)PASSUPVECTOR;
    
    pass_up_vector->tlb_refill_handler  = (memaddr)TLBRefillHandler;
    pass_up_vector->tlb_refill_stackPtr = (memaddr)KERNELSTACK;
    pass_up_vector->exception_handler   = (memaddr)exceptionHandler;
    pass_up_vector->exception_stackPtr  = (memaddr)KERNELSTACK;
}

/**
 * @brief Inizializza i semafori
*/
static void _initDeviceSemaphores() {
    semaphore_it = 0;
    for (int i = 0; i<TOTAL_IO_DEVICES; i++) {
        semaphore_devices[i] = 0;
    }
}

/**
 * @brief Crea il primo processo.
 * @return Puntatore al PCB del processo creato.
*/
static pcb_t *_createFirstProcess() {
    pcb_t *first_proc = allocPcb();

    first_proc->p_prio = PROCESS_PRIO_LOW;
    RAMTOP(first_proc->p_s.reg_sp);
    // Interrupt abilitati + PLT abilitato + Kernel mode
    first_proc->p_s.status = ALLOFF | IMON | IEPON | TEBITON;
    first_proc->p_s.reg_t9 = first_proc->p_s.pc_epc = (memaddr)test;

    return first_proc;
}

void main() {
    _initPassUpVector();
    initPcbs();
    initASL();
    process_count = 0;
    softblocked_count = 0;
    mkEmptyProcQ(&high_readyqueue);
    mkEmptyProcQ(&low_readyqueue);
    curr_process = NULL;
    _initDeviceSemaphores();
    resetIntervalTimer();

    insertProcQ(&low_readyqueue, _createFirstProcess());
    process_count++;

    scheduler();
    PANIC();
}