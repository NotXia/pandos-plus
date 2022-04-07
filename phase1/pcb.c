#include "pcb.h"

static pcb_t pcbFree_table[MAXPROC];
static struct list_head pcbFree_h;

static pid_t curr_pid;
static struct list_head pid_list_h;
#define FREE_PCB_PID -1

/**
 * @brief Inizializza le strutture dati.
*/
void initPcbs() {
    INIT_LIST_HEAD(&pcbFree_h);
    INIT_LIST_HEAD(&pid_list_h);
    curr_pid = 1;

    for (int i=MAXPROC-1; i>=0; i--) {
        list_add(&pcbFree_table[i].p_list, &pcbFree_h);
        pcbFree_table[i].p_pid = FREE_PCB_PID;
    }
}

/**
 * @brief Inserisce uno specifico PCB nella lista dei PCB liberi.
 * @param p Puntatore del PCB da inserire.
*/
void freePcb(pcb_t *p) {
    list_add(&p->p_list, &pcbFree_h);
    list_del(&p->pid_list);
    p->p_pid = FREE_PCB_PID;
}


/**
 * @brief Genera un pid per un processo.
 * @return Restituisce un pid assegnabile.
*/
static pid_t _generatePid() {
    // Gestione wraparound
    if (curr_pid <= 0) { curr_pid = 1; }

    // Gestione collisioni
    int curr_greater_pid = container_of(pid_list_h.prev, pcb_t, pid_list)->p_pid; // L'ultimo elemento della lista dei pid è il più grande
    if (curr_greater_pid >= curr_pid) { // Se minore, sicuramente non ci sono collisioni
        while (getProcessByPid(curr_pid) != NULL) {
            curr_pid++;
        }
        curr_pid--; // Per evitare il doppio incremento
    }
    
    return curr_pid++;
}

/**
 * @brief Imposta i campi generali di un PCB inizializzandoli opportunamente.
 * @param pcb Puntatore al PCB da inizializzare.
*/
static void _initPcb(pcb_t *pcb) {
    INIT_LIST_HEAD(&pcb->p_list);
    pcb->p_parent = NULL;
    INIT_LIST_HEAD(&pcb->p_child);
    INIT_LIST_HEAD(&pcb->p_sib);
    pcb->p_time = 0;
    pcb->p_semAdd = NULL;
    pcb->p_supportStruct = NULL;
    pcb->p_s.entry_hi = pcb->p_pid = _generatePid();
}

static void _addPid(pcb_t *p) {
    int inserted = FALSE;
    pcb_t *iter;

    // Inserimento per mantenere la lista ordinata in senso crescente per pid
    // Dato che i pid sono crescenti, si inserisce scorrendo la lista al contrario per ottimizzare
    list_for_each_entry_reverse(iter, &pid_list_h, pid_list) {
        if (p->p_pid >= iter->p_pid) {
            __list_add(&p->pid_list, &iter->pid_list, iter->pid_list.next);
            inserted = TRUE;
            break;
        }
    }

    // Nel caso in cui si raggiunga la fine della lista senza che avvenga l'inserimento o se la lista è vuota
    if (inserted == FALSE) {
        list_add_tail(&p->pid_list, &pid_list_h);
    }
}

/**
 * @brief Rimuove un elemento dalla lista dei PCB liberi e lo restituisce inizializzato. Vanno ancora inizializzati i parametri specifici del processo
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
        _addPid(out);

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
 * @return Il PCB rimosso. NULL se la coda è vuota o se il PCB non appartiene alla coda indicata.
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

/**
 * @brief Cerca il processo dato un pid.
 * @param pid Pid del processo da cercare.
 * @return Il puntatore al processo. NULL se non esiste.
*/
pcb_t *getProcessByPid(pid_t pid) {
    pcb_t *iter;

    // Inserimento per mantenere la lista ordinata in senso crescente per pid
    list_for_each_entry(iter, &pid_list_h, pid_list) {
        if (iter->p_pid == pid) { return iter; }
        if (iter->p_pid > pid) { return NULL; }
    }
}
