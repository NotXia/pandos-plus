#include "asl.h"
#include "pcb.h"
#include <scheduler.h>
#include <initial.h>
#include <exceptions.h>

static semd_t semd_table[MAXPROC];  // Allocazione dei semafori
static struct list_head semdFree_h; // Lista di semafori liberi
static struct list_head semd_h;     // Lista di semafori attivi (ASL)

/**
 * @brief Inserisce un semaforo nella lista dei semafori liberi.
 * @param sem Puntatore al semaforo da inserire.
*/
static void _addFreeSemaphore(semd_t *sem) {
    list_add(&sem->s_link, &semdFree_h);
}

/**
 * @brief Rimuove e restituisce un semaforo libero.
 * @return Puntatore al semaforo. NULL se non ci sono semafori liberi.
*/
static semd_t *_getFreeSemaphore() {
    if (list_empty(&semdFree_h)) { return NULL; }

    // Rimozione in testa
    semd_t *sem = container_of(semdFree_h.next, semd_t, s_link);
    list_del(&sem->s_link);

    return sem;
}

/**
 * @brief Inserisce un semaforo nella lista dei semafori attivi.
 * @param new_sem Puntatore al semaforo da inserire.
*/
static void _addActiveSemaphore(semd_t *new_sem) {
    int inserted = FALSE;
    semd_t *iter;

    // Inserimento per mantenere la lista ordinata in senso crescente per chiave
    list_for_each_entry(iter, &semd_h, s_link) {
        if (new_sem->s_key < iter->s_key) {
            __list_add(&new_sem->s_link, iter->s_link.prev, &iter->s_link);
            inserted = TRUE;
            break;
        }
    }

    // Nel caso in cui si raggiunga la fine della lista senza che avvenga l'inserimento o se la lista è vuota
    if (inserted == FALSE) {
        list_add_tail(&new_sem->s_link, &semd_h);
    }
}

/**
 * @brief Cerca e restituisce un semaforo attivo ricercato per chiave.
 * @param s_key Puntatore alla chiave del semaforo da cercare
 * @return Puntatore al semaforo, se trovato. NULL altrimenti.
*/
static semd_t *_peekActiveSemaphore(int *s_key) {
    semd_t *iter;

    list_for_each_entry(iter, &semd_h, s_link) {
        // Semaforo trovato
        if (iter->s_key == s_key) { return iter; }

        // Semaforo inesistente
        if (iter->s_key > s_key) { break; }
    }

    return NULL;
}

/**
 * @brief Restituisce un semaforo inizializzato estratto dai semafori liberi e inserito nella ASL.
 * @param s_key Chiave del nuovo semaforo da inizializzare
 * @return Puntatore al semaforo inizializzato. NULL se non ci sono semafori liberi.
*/
static semd_t *_initSemaphore(int *s_key) {
    semd_t *new_sem = _getFreeSemaphore();

    // Se non ci sono semafori liberi
    if (new_sem == NULL) { return NULL; } 

    // Inizializzazione
    new_sem->s_key = s_key;
    mkEmptyProcQ(&new_sem->s_procq);

    _addActiveSemaphore(new_sem);

    return new_sem;
}

/**
 * @brief Verifica se un semaforo è ancora attivo, in caso negativo viene rimosso dalla ASL e inserito nella lista dei semafori liberi.
 *        Nota: non controlla che il semaforo sia effettivamente nella ASL.
 * @param toCheckSem Puntatore al semaforo da aggiornare
*/
static void _updateActiveSemaphore(semd_t *toCheckSem) {
    if (emptyProcQ(&toCheckSem->s_procq)) {
        // Rimuove dalla ASL
        list_del(&toCheckSem->s_link);

        // Inserisce nella lista dei semafori liberi
        _addFreeSemaphore(toCheckSem);
    }
}



/**
 * @brief Inizializza le strutture dati.
*/
void initASL() {
    INIT_LIST_HEAD(&semdFree_h);
    INIT_LIST_HEAD(&semd_h);

    // Inserisce ogni locazione disponibile per i semafori nella lista dei semafori liberi
    for (int i=MAXPROC-1; i>=0; i--) {
        _addFreeSemaphore(&semd_table[i]);
    }
}

/**
 * @brief Inserisce un PCB alla coda dei processi bloccati di un semaforo identificato per chiave.
 *        Se il semaforo non è attivo, viene inizializzato.
 * @param semAdd Puntatore alla chiave del semaforo a cui aggiungere il PCB
 * @param p      Puntatore al PCB da inserire nella lista dei processi bloccati
 * @return TRUE se il semaforo non è attivo e non ci sono semafori liberi. FALSE altrimenti.
*/
int insertBlocked(int *semAdd, pcb_t *p) {
    semd_t *sem = _peekActiveSemaphore(semAdd);
    
    if (sem == NULL) { // Semaforo da inizializzare
        sem = _initSemaphore(semAdd);

        // Non ci sono semafori liberi
        if (sem == NULL) { return TRUE; }
    }

    insertProcQ(&sem->s_procq, p);
    p->p_semAdd = semAdd; // Bisogna specificare nel PCB su che semaforo è bloccato

    return FALSE;
}

/**
 * @brief Rimuove e restituisce il primo PCB bloccato associato ad un semaforo ricercato per chiave. 
 *        Se dopo la rimozione non ci sono più processi bloccati, il semaforo viene rimosso dalla ASL e inserito nei semafori liberi.
 * @param semAdd Puntatore alla chiave del semaforo
 * @return Il PCB rimosso se esiste. NULL altrimenti.
*/
pcb_t *removeBlocked(int *semAdd) {
    semd_t *sem = _peekActiveSemaphore(semAdd);
    if (sem == NULL) { return NULL; } // Il semaforo non esiste

    pcb_t *pcb = removeProcQ(&sem->s_procq);
    _updateActiveSemaphore(sem);
    
    return pcb;
}

/**
 * @brief Rimuove e restituisce uno specifico PCB bloccato associato ad un semaforo ricercato per chiave.
 *        Se dopo la rimozione non ci sono più processi bloccati, il semaforo viene rimosso dalla ASL e inserito nei semafori liberi.
 * @param p Puntatore al PCB da rimuovere
 * @return Il PCB rimosso se esiste. NULL altrimenti.
*/
pcb_t *outBlocked(pcb_t *p) {
    semd_t *sem = _peekActiveSemaphore(p->p_semAdd);
    if (sem == NULL) { return NULL; } // Il semaforo non esiste
    
    pcb_t *pcb = outProcQ(&sem->s_procq, p);
    _updateActiveSemaphore(sem);
    
    return pcb;
}

/**
 * @brief Restituisce il primo PCB bloccato associato ad un semaforo ricercato per chiave.
 * @param semAdd Puntatore alla chiave del semaforo
 * @return Il PCB in testa se esiste. NULL altrimenti.
*/
pcb_t *headBlocked(int *semAdd) {
    semd_t *sem = _peekActiveSemaphore(semAdd);
    if (sem == NULL) { return NULL; } // Il semaforo non esiste

    return headProcQ(&sem->s_procq);
}


/**
 * @brief Esegue la P su un semaforo binario.
 * @param sem Puntatore del semaforo.
*/
void P(int *sem) {
    if (*sem == 0) {
        if (insertBlocked(sem, curr_process)) { PANIC(); } // Non ci sono semafori disponibili
        setProcessBlocked(curr_process, PREV_PROCESSOR_STATE);
        scheduler();
    }
    else if (!headBlocked(sem) != NULL) {
        pcb_t *ready_proc = removeBlocked(sem);
        setProcessReady(ready_proc);
    }
    else {
        *sem = 0;
    }
}

/**
 * @brief Esegue la V su un semaforo binario.
 * @param sem Puntatore del semaforo.
 * @return Il processo sbloccato. NULL se non esiste.
*/
pcb_t *V(int *sem) {
    pcb_t *ready_proc;
    
    if (*sem == 1) {
        if (insertBlocked(sem, curr_process)) { PANIC(); } // Non ci sono semafori disponibili
        setProcessBlocked(curr_process, PREV_PROCESSOR_STATE);
        scheduler();
    }
    else if (!headBlocked(sem) != NULL) {
        ready_proc = removeBlocked(sem);
        setProcessReady(ready_proc);
    }
    else {
        *sem = 1;
    }

    return ready_proc;
}