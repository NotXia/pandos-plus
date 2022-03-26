#include <initial.h>
#include <asl.h>
#include <pandos_const.h>
#include <pandos_types.h>
#include <execptions.h>
#include <interrupts.h>
#include <scheduler.h>


/**
 * @brief Genera pid di un processo
 * @return Restituisce un pid assegnabile progressivo
*/
int generatePid() {
    return curr_pid++;
}

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
static void _initSemaphores() {
    semaphore_plt = 0;
    semaphore_bus = 0;
    for (int i = 0; i<8; i++) {
        semaphore_disk[i] = 0;
        semaphore_flashdrive[i] = 0;
        semaphore_network[i] = 0;
        semaphore_printer[i] = 0;
        semaphore_terminal[i] = 0;
        semaphore_terminal[i+8] = 0;
    }
}

/**
 * @brief Crea il primo processo
*/
static void _createFirstProcess() {
    pcb_t *first_proc = allocPcb();

    first_proc->p_supportStruct = NULL;
    first_proc->p_prio = PROCESS_PRIO_LOW;
    first_proc->p_pid = generatePid();
    first_proc->p_s.entry_hi = first_proc->p_pid;
    RAMTOP(first_proc->p_s.reg_sp);
    first_proc->p_s.status = ALLOFF | IMON | TEBITON | IEPON
    first_proc->p_s.pc_epc = (memaddr)test;
    first_proc->p_s.s_t9 = (memaddr)test;

    return first_proc;
}

int main() {
    _initPassUpVector();

    initPcbs();
    initASL();

    process_count = 0;
    softblocked_count = 0;
    mkEmptyProcQ(high_readyqueue);
    mkEmptyProcQ(low_readyqueue);

    curr_process = NULL;

    _initSemaphores();

    LDIT(100);

    curr_pid = 1;

    pcb_t *first_proc = _createFirstProcess();

    scheduler();
}