#include "vmSupport.h"
#include <pandos_types.h>
#include <listx.h>
#include <initial.h>
#include <umps3/umps/cp0.h>
#include <umps3/umps/libumps.h>

static int swap_pool_sem;
static swap_t swap_pool_table[POOLSIZE];
static int swap_pool_index; // Indice della pagina più datata

/**
 * @brief Inizializza le strutture dati per la swap pool.
*/
void initSwapStructs() {
    swap_pool_sem = 1;
    swap_pool_index = 0;

    for (int i=0; i<POOLSIZE; i++) {
        swap_pool_table[i].sw_asid = NOPROC;
        swap_pool_table[i].sw_pageNo = 0;
        swap_pool_table[i].sw_pte = NULL;
    }
}

/**
 * @brief Calcola e restituisce l'indice della entry nella page table.
 * @param vpn campo VPN (indirizzo logico associato al frame).
 * @return L'indice della entry nella page table del processo.
*/
static int _getPageIndex(memaddr vpn) {
    if (vpn == 0xBFFFF) {
        return 31;
    }
    return vpn % 0x8000;
}

/**
 * @brief Gestore TLB refill.
*/
void TLBRefillHandler() {
    int page_index = _getPageIndex(ENTRYHI_GET_VPN(PREV_PROCESSOR_STATE->entry_hi));
    pteEntry_t pt_entry = curr_process->p_supportStruct->sup_privatePgTbl[page_index];

    setENTRYHI(pt_entry.pte_entryHI);
    setENTRYLO(pt_entry.pte_entryLO);
    TLBWR();

    LDST(PREV_PROCESSOR_STATE);
}

/**
 * @brief Seleziona e restituisce un frame utilizzabile.
 * @returns Un frame utilizzabile.
*/
static swap_t* _pager() {
    swap_t *to_swap = &swap_pool_table[swap_pool_index];
    swap_pool_index++;
    if (swap_pool_index >= POOLSIZE) { swap_pool_index = 0; }

    return to_swap;
}

/**
 * @brief Gestore delle eccezioni TLB-invalid.
*/
static void _TLBInvalidHandler() {

}

/**
 * @brief Gestore delle eccezioni TLB.
*/
void TLBExceptionHandler() {
    support_t *support_structure = (support_t *)SYSCALL(GETSUPPORTPTR, NULL, NULL, NULL);
    
    switch (CAUSE_GET_EXCCODE(support_structure->sup_exceptState[0].cause)) {
        case TLBMOD:
            break;

        case TLBINVLDL:
        case TLBINVLDS:
            _TLBInvalidHandler();
            break;
    }
}