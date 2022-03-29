#include <scheduler.h>
#include <initial.h>
#include <umps3/umps/libumps.h>

pcb_t *to_ignore = NULL; // Per gestire i processi che chiamano yield

/**
 * @brief Seleziona il prossimo processo da mandare avanti.
 * @param next_pcb Verrà inserito il processo successivo.
 * @param prio Verrà inserita la priorità del processo.
*/
static void _getNextProcess(pcb_t *next_pcb, int *prio) {
    pcb_t *next_proc = NULL;

    if (!emptyProcQ(high_readyqueue)) {
        if (headProcQ(high_readyqueue) != to_ignore || emptyProcQ(low_readyqueue)) {
            next_proc = removeProcQ(high_readyqueue);
            *prio = PROCESS_PRIO_HIGH;
        }
    }

    if (next_proc == NULL) {
        next_proc = removeProcQ(low_readyqueue);
        *prio = PROCESS_PRIO_LOW;
    }

    to_ignore = NULL;

    return next_proc;
}

/**
 * @brief Manda in esecuzione il prossimo processo oppure gestisce i casi di attesa/errore.
*/
void scheduler() {
    if (process_count == 0) { HALT(); }

    pcb_t *next_proc;
    int next_prio;

    _getNextProcess(next_proc, &next_prio);

    if (next_proc == NULL) {
        if (softblocked_count > 0) {
            // Abilita interrupt + disabilita PLT
            setSTATUS((getSTATUS() | IECON | IMON) & ~TEBITON);
            WAIT(); 
        }
        else { 
            PANIC(); 
        }
    }
    else { // Esiste almeno un processo ready
        curr_process = next_proc;

        if (next_prio == PROCESS_PRIO_LOW) { setTIMER(TIMESLICE); }
        LDST(&next_proc->p_s);
    }

}