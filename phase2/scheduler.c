#include <scheduler.h>
#include <umps3/umps/libumps.h>
#include <initial.h>
#include <utilities.h>

/**
 * @brief Seleziona il prossimo processo da mandare avanti.
 * @param next_pcb Verrà inserito il processo successivo.
 * @param prio Verrà inserita la priorità del processo.
*/
static pcb_t *_getNextProcess(int *prio) {
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
            *prio = PROCESS_PRIO_HIGH;
        }
    }

    // Prova ad estrare un processo a bassa priorità se non è riuscita ad estrarne uno ad alta
    if (next_proc == NULL) {
        next_proc = removeProcQ(&low_readyqueue);
        *prio = PROCESS_PRIO_LOW;
    }

    process_to_skip = NULL;
    return next_proc;
}


/**
 * @brief Manda in esecuzione il prossimo processo oppure gestisce i casi di attesa/errore.
*/
void scheduler() {
    if (process_count == 0) { HALT(); }

    pcb_t *next_proc = NULL;
    int next_prio;
    next_proc = _getNextProcess(&next_prio);

    if (next_proc == NULL) {
        if (softblocked_count > 0) {
            curr_process = NULL;
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

        if (next_prio == PROCESS_PRIO_LOW) {
            next_proc->p_s.status = (next_proc->p_s.status) | TEBITON;
            setTIMER(TIMESLICE); 
        }
        else {
            // setTIMER(0xFFFFFFFF);
            next_proc->p_s.status = (next_proc->p_s.status) & ~TEBITON;
        }
        timerFlush();

        LDST(&next_proc->p_s);
    }

}

/**
 * @brief Cambia lo stato del processo a bloccato.
 * @param p Puntatore al PCB del processo.
 * @param state Puntatore allo stato da salvare nel processo.
*/
void setProcessBlocked(pcb_t *p, state_t *state) {
    // Oss: dato che non è prevista la possibilità di bloccare un processo nella ready queue, l'unico che è possibile bloccare è il corrente
    // Controllo implementato per maggiore coerenza ma la condizione non si verificherà mai
    if (p != curr_process) { outProcQ(GET_READY_QUEUE(p->p_prio), p); }
    
    curr_process->p_s = *state;
    curr_process->p_time += timerFlush();
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