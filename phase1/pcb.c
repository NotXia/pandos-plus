#include "pcb.h"

static pcb_t pcbFree_table[MAX_PROC];
static LIST_HEAD(pcbFree_h);

/**
 * @brief Inizializza le strutture dati.
*/
void initPcbs() {
    for (int i=MAXPROC-1; i<=0; i--) {
        list_add(&pcbFree_table[i].p_list, &pcbFree_h);
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
 * @brief Rimuove un elemento dalla lista dei PCB liberi e lo restituisce inizializzato.
 * @return L'elemento rimosso. NULL se la lista dei PCB liberi è vuota.
*/
pcb_t *allocPcb() {
    if (list_empty(&pcbFree_h) == TRUE) {
        return NULL;
    } else {
        pcb_t *out = container_of(&pcbFree_h.next, struct pcb_t, p_list);
        list_del(&out->p_list);

        // Inizializzazione
        INIT_LIST_HEAD(&out->p_list)
        out->p_parent = NULL;
        INIT_LIST_HEAD(&out->p_child)
        INIT_LIST_HEAD(&out->p_sib)
        out->p_s = 0;    
        out->p_time = 0; 
        out->p_semAdd = NULL;
        
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
void insertProcQ(struct list_head *head, pcb *p) {
    list_add_tail(&p->p_list, head);
}

/**
 * @brief Restituisce l'elemento di testa della coda dei processi.
 * @param head Puntatore alla testa della coda.
 * @return La testa della coda. NULL se la coda è vuota.
*/
pcb_t *headProcQ(struct list_head *head) {
    if (list_empty(head) == TRUE) {
        return NULL;
    }
    else{    
        return container_of(head, struct pcb_t, p_list);
    }
}

/**
 * @brief Rimuove il primo elemento dalla coda dei processi e lo restituisce.
 * @param head Puntatore alla testa della coda.
 * @return Puntatore all'elemento rimosso. NULL se la coda è vuota.
*/
pcb_t *removeProcQ(struct list_head *head) {
    if (list_empty(head) == TRUE) {
        return NULL;
    } else {
        pcb_t *out = container_of(head, struct pcb_t, p_list);
        list_del(&out->p_list);
        return out;
    }
}

/**
 * @brief Rimuove il PCB dalla coda dei processi e lo restituisce.
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
    list_add(&p->p_sib, &prnt->p_child);
    p->p_parent = prnt;
}

/**
 * @brief Rimuove il primo figlio del PCB indicato.
 * @param p Puntatore al PCB.
 * @return Il PCB rimosso. NULL se p non ha figli.
*/
pcb_t *removeChild(pcb_t *p) {
    if (emptyChild(p) == TRUE) {
        return NULL;
    } else {
        pcb_t *out = container_of(p->p_child.next, struct pcb_t, p_sib);
        list_del(&out->p_sib);
        out->p_parent = NULL;
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
        list_del(p->p_sib);
        p->p_parent = NULL;
        return p;
    }
}