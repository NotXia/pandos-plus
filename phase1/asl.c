#include "asl.h"

static semd_t semd_table[MAXPROC];  // Allocazione dei semafori
static LIST_HEAD(semdFree_h);       // Lista di semafori liberi
static LIST_HEAD(semd_h);           // Lista di semafori attivi (ASL)


/**
 * @brief Inizializza le strutture dati.
*/
void initASL() {
    // Inizializzazione sentinella
    semdFree_h.prev = &semd_table[MAXPROC-1].s_link;
    semdFree_h.next = &semd_table[0].s_link;

    // Inizializzazione elementi della lista di semafori liberi
    semd_table[0].s_link.prev = &semdFree_h;
    semd_table[0].s_link.next = &semd_table[1].s_link;
    for (int i=1; i<MAXPROC-1; i++) {
        semd_table[i].s_link.prev = &semd_table[i-1].s_link;
        semd_table[i].s_link.next = &semd_table[i+1].s_link;
    }
    semd_table[MAXPROC-1].s_link.prev = &semd_table[MAXPROC-2].s_link;
    semd_table[MAXPROC-1].s_link.next = &semdFree_h;
}

/**
 * @brief Inizializza e restituisce un puntatore ad un semaforo libero.
 * @param s_key Puntatore alla chiave del semaforo da inizializzare
 * @return Puntatore al semaforo inizializzato. NULL se non ci sono semafori liberi.
*/
static semd_t *_initSemaphore(int *s_key) {
    if (list_empty(&semdFree_h) == TRUE) { return NULL; }

    // Estrae un semaforo libero
    semd_t *new_sem = container_of(semdFree_h.next, struct semd_t, s_link);
    list_del(new_sem);

    // Inizializzazione
    new_sem->s_key = s_key;
    INIT_LIST_HEAD(&new_sem->s_procq);

    return new_sem;
}

/**
 * @brief Inserisce un semaforo nella lista dei semafori attivi (se ha processi bloccati).
 * @param new_sem Puntatore al semaforo da inserire.
 * @return TRUE se il semaforo non ha processi bloccati. FALSE altrimenti.
*/
static int _addActiveSemaphore(semd_t *new_sem) {
    struct semd_t *iter;

    // Controlla che il semaforo abbia almeno un processo bloccato
    if (list_empty(&new_sem->s_procq) == TRUE) { return TRUE; }

    // Inserimento per mantenere la lista ordinata in senso non decrescente
    list_for_each_entry(iter, &semd_h, s_link) {
        if (new_sem->s_key < iter->s_key) {
            __list_add(&new_sem->s_link, iter->s_link.prev, &iter->s_link);
            break;
        }
    }

    return FALSE;
}

/**
 * @brief Cerca e restituisce un semaforo attivo ricercato per chiave.
 * @param s_key Puntatore alla chiave del semaforo da cercare
 * @return Puntatore al semaforo, se trovato. NULL altrimenti.
*/
static semd_t *_getSemaphore(int *s_key) {
    struct semd_t *iter;

    list_for_each_entry(iter, &semd_h, s_link) {

        // Semaforo esistente
        if (iter->s_key == s_key) { return iter; }

        // Semaforo inesistente
        if (iter->s_key > s_key) { break; }
    }

    return NULL;
}

/**
 * @brief Inserisce un PCB alla lista dei processi bloccati di un semaforo identificato per chiave.
 *        Se il semaforo non è attivo, viene inizializzato.
 * @param semAdd Puntatore alla chiave del semaforo a cui aggiungere il PCB
 * @param p      Puntatore al PCB da inserire nella lista dei bloccati
 * @return TRUE in caso il semaforo non è attivo e non ci sono semafori liberi. FALSE altrimenti.
*/
int insertBlocked(int *semAdd, pcb_t *p) {
    struct semd_t *sem = _getSemaphore(semAdd);
    
    if (sem != NULL) { // Semaforo esistente
        list_add_tail(p, &sem->s_procq);
    }
    else { // Semaforo da inizializzare
        sem = _initSemaphore(semAdd);
        if (sem == NULL) { return TRUE; }

        list_add_tail(p, &sem->s_procq);
        _addActiveSemaphore(sem);
    }

    return FALSE;
}

pcb_t *removeBlocked(int *semAdd) {

}

pcb_t *outBlocked(pcb_t *p) {

}

pcb_t *headBlocked(int *semAdd) {

}