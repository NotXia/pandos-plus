#include "pcb.h"

static pcb_t pcbFree_table[MAXPROC];
static struct list_head pcbFree_h;

/**
 * @brief Inizializza le strutture dati.
*/
void initPcbs() {
    INIT_LIST_HEAD(&pcbFree_h);

    for (int i=MAXPROC-1; i>=0; i--) {
        freePcb(&pcbFree_table[i]);
    }
}

/**
 * @brief Inserisce uno specifico PCB nella lista dei PCB liberi.
 * @param p Puntatore del PCB da inserire.
*/
void freePcb(pcb_t *p) {
    list_add(&p->p_list, &pcbFree_h);
}

/**
 * @brief Imposta i campi di un PCB inizializzandoli opportunamente a 0, NULL o lista vuota.
 * @param pcb Puntatore al PCB da inizializzare.
*/
static void _initPcb(pcb_t *pcb) {
    INIT_LIST_HEAD(&pcb->p_list);
    pcb->p_parent = NULL;
    INIT_LIST_HEAD(&pcb->p_child);
    INIT_LIST_HEAD(&pcb->p_sib);
    pcb->p_s.entry_hi = 0;
    pcb->p_s.cause = 0;
    pcb->p_s.status = 0;
    pcb->p_s.pc_epc = 0;
    for (int i=0; i<STATE_GPR_LEN; i++) { pcb->p_s.gpr[i] = 0; }
    pcb->p_s.hi = 0;
    pcb->p_s.lo = 0;
    pcb->p_time = 0; 
    pcb->p_semAdd = NULL;
}

/**
 * @brief Rimuove un elemento dalla lista dei PCB liberi e lo restituisce inizializzato.
 * @return L'elemento rimosso. NULL se la lista dei PCB liberi è vuota.
*/
pcb_t *allocPcb() {
    if (list_empty(&pcbFree_h)) {
        return NULL;
    } else {
        // Rimozione in testa
        pcb_t *out = container_of(pcbFree_h.next, pcb_t, p_list);
        list_del(&out->p_list);

        _initPcb(out);
        
        return out;
    }
}

/**
 * @brief Inizializza una lista di PCB vuota.
 * @param head Puntatore alla testa della lista.
*/
void mkEmptyProcQ(struct list_head *head) {
    INIT_LIST_HEAD(head);
}

/**
 * @brief Controlla se la lista è vuota.
 * @param head Puntatore alla testa della lista.
 * @return TRUE se la lista è vuota. FALSE altrimenti.
*/
int emptyProcQ(struct list_head *head) {
    return list_empty(head);
}

/**
 * @brief Inserisce un PCB nella coda dei processi.
 * @param head Puntatore alla testa della lista.
 * @param p Puntatore al PCB da inserire.
*/
void insertProcQ(struct list_head *head, pcb_t *p) {
    list_add_tail(&p->p_list, head);
}

/**
 * @brief Restituisce l'elemento di testa della coda dei processi.
 * @param head Puntatore alla testa della coda.
 * @return La testa della coda. NULL se la coda è vuota.
*/
pcb_t *headProcQ(struct list_head *head) {
    if (emptyProcQ(head)) {
        return NULL;
    }
    else{    
        return container_of(head->next, pcb_t, p_list);
    }
}

/**
 * @brief Rimuove il primo elemento dalla coda dei processi e lo restituisce.
 * @param head Puntatore alla testa della coda.
 * @return Puntatore all'elemento rimosso. NULL se la coda è vuota.
*/
pcb_t *removeProcQ(struct list_head *head) {
    if (emptyProcQ(head)) {
        return NULL;
    } else {
        pcb_t *out = headProcQ(head);
        list_del(&out->p_list);

        return out;
    }
}

/**
 * @brief Rimuove il PCB dalla coda dei processi specificata e lo restituisce.
 * @param head Puntatore alla testa della coda.
 * @param p Puntatore del PCB da rimuovere dalla coda.
 * @return Il PCB rimosso. NULL se la coda è vuota.
*/
pcb_t *outProcQ(struct list_head *head, pcb_t *p) {
    pcb_t *pos;

    // Verifica che il PCB sia effettivamente nella coda indicata
    list_for_each_entry(pos, head, p_list) {
        if (pos == p) {
            list_del(&p->p_list);
            return p;
        }
    }
    return NULL;
}

/**
 * @brief Controlla se il PCB indicato non ha figli.
 * @param p Puntatore al PCB.
 * @return TRUE se non ha figli. FALSE altrimenti.
*/
int emptyChild(pcb_t *p) {
    return list_empty(&p->p_child);
}

/**
 * @brief Inserisce il PCB indicato come figlio di un altro PCB.
 * @param pnrt Puntatore al PCB padre.
 * @param p Puntatore al PCB figlio.
*/
void insertChild(pcb_t *prnt, pcb_t *p) {
    list_add_tail(&p->p_sib, &prnt->p_child);
    p->p_parent = prnt;
}

/**
 * @brief Dato un PCB, lo rimuove dall'albero a cui appartiene
 * @param pcb Puntatore al PCB da rimuovere.
*/
static void _removeFromTree(pcb_t *pcb) {
    list_del(&pcb->p_sib);
    pcb->p_parent = NULL;
}

/**
 * @brief Rimuove il primo figlio del PCB indicato.
 * @param p Puntatore al PCB.
 * @return Il PCB rimosso. NULL se p non ha figli.
*/
pcb_t *removeChild(pcb_t *p) {
    if (emptyChild(p)) {
        return NULL;
    } else {
        pcb_t *out = container_of(p->p_child.next, pcb_t, p_sib);
        
        _removeFromTree(out);

        return out;
    }
}

/**
 * @brief Rimuove il PCB indicato dalla lista dei figli del padre.
 * @param p Puntatore al PCB.
 * @return Il PCB Rimosso. NULL se p non ha un padre.
*/
pcb_t *outChild(pcb_t *p) {
    if (p->p_parent == NULL) {
        return NULL;
    } else {
        _removeFromTree(p);

        return p;
    }
}