#include <scheduler.h>
#include <umps3/umps/libumps.h>
#include <initial.h>
#include <utilities.h>

/**
 * @brief Seleziona il prossimo processo da mandare in esecuzione.
 * @return Il processo selezionato.
*/
static pcb_t *_getNextProcess() {
    pcb_t *next_proc = NULL;

    /*
        Prova ad estrare un processo ad alta priorità
        Estrae se:
        - il processo in testa è diverso da quello da ignorare (yield)
        - la coda dei processi a bassa priorità è vuota e quindi necessariamente seleziona un processo ad alta priorità (anche se da ignorare)
    */
    if (!emptyProcQ(&high_readyqueue)) {
        if (headProcQ(&high_readyqueue) != process_to_skip || emptyProcQ(&low_readyqueue)) {
            next_proc = removeProcQ(&high_readyqueue);
        }
    }

    // Prova ad estrare un processo a bassa priorità se non è riuscita ad estrarne uno ad alta
    if (next_proc == NULL) {
        next_proc = removeProcQ(&low_readyqueue);
    }

    process_to_skip = NULL;
    return next_proc;
}


/**
 * @brief Manda in esecuzione il prossimo processo oppure gestisce i casi di attesa/deadlock.
*/
void scheduler() {
    if (process_count == 0) { HALT(); }

    curr_process = _getNextProcess();

    if (curr_process == NULL) {
        if (softblocked_count > 0) { // Processi in attesa di I/O
            // Abilita interrupt + disabilita PLT
            setSTATUS((getSTATUS() | IECON | IMON) & ~TEBITON);
            WAIT(); 
        }
        else { // Deadlock
            PANIC();
        }
    }
    else { // Esiste almeno un processo ready
        if (curr_process->p_prio == PROCESS_PRIO_LOW) {
            curr_process->p_s.status = (curr_process->p_s.status) | TEBITON; // PLT attivato
            startPLT();
        }
        else {
            curr_process->p_s.status = (curr_process->p_s.status) & ~TEBITON; // PLT disattivato
        }
        timerFlush();

        LDST(&curr_process->p_s);
    }

}

/**
 * @brief Cambia lo stato del processo a bloccato.
 * @param p Puntatore al PCB del processo.
 * @param state Puntatore allo stato da salvare nel processo.
*/
void setProcessBlocked(pcb_t *p, state_t *state) {
    /* Oss: dato che non è prevista la possibilità di bloccare un processo nella ready queue, l'unico che è possibile bloccare è il corrente
            Quindi non è necessario fare nessun controllo sulle ready queue */
    
    curr_process->p_s = *state;
    updateProcessCPUTime();
}


/**
 * @brief Cambia lo stato del processo a ready.
 * @param p Puntatore al PCB del processo.
*/
void setProcessReady(pcb_t *p) {
    if (IS_ALIVE(p)) {
        insertProcQ(GET_READY_QUEUE(p->p_prio), p);
    }
}