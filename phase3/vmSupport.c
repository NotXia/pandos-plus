#include "vmSupport.h"
#include <pandos_types.h>
#include <listx.h>
#include <initial.h>
#include <umps3/umps/cp0.h>
#include <umps3/umps/libumps.h>
#include <umps3/umps/types.h>

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
static swap_t* _getFrame() {
    swap_t *to_swap = &swap_pool_table[swap_pool_index];
    swap_pool_index++;
    if (swap_pool_index >= POOLSIZE) { swap_pool_index = 0; }

    return to_swap;
}

/**
 * @brief Gestore delle eccezioni TLB-invalid.
*/
static void _TLBInvalidHandler(support_t *support_structure) {
    SYSCALL(PASSEREN, (int)&swap_pool_sem, NULL, NULL);

    int new_asid = ENTRYHI_GET_ASID(PREV_PROCESSOR_STATE->entry_hi);
    int missing_page_index = _getPageIndex(ENTRYHI_GET_VPN(PREV_PROCESSOR_STATE->entry_hi));
    swap_t *new_frame = _getFrame();
    memaddr new_frame_start_address = FRAMEPOOLSTART + (swap_pool_index-1)*PAGESIZE;

    if (new_frame->sw_asid != NOPROC) {
        setSTATUS(getSTATUS() & ~IECON); // Disabilita interrupt
        new_frame->sw_pte->pte_entryLO = new_frame->sw_pte->pte_entryLO & ~VALIDON;
        TLBCLR(); // TODO Ottimizzare
        setSTATUS(getSTATUS() | IECON | IMON); // Abilita interrupt

        // Salvataggio della vecchia pagina nel flash drive del proprietario
        dtpreg_t *flash_dev_reg = (dtpreg_t *)DEV_REG_ADDR(4, new_frame->sw_asid);
        flash_dev_reg->data0 = new_frame_start_address;
        int command = (_getPageIndex(new_frame->sw_pageNo) << 8) + FLASHWRITE;
        int res = SYSCALL(DOIO, &flash_dev_reg->command, command, NULL);
        if (res == FLASH_WRITE_ERROR) {
            // Trap
        }
    }

    // Caricamento della nuova pagina in memoria
    dtpreg_t *flash_dev_reg = (dtpreg_t *)DEV_REG_ADDR(4, new_asid);
    flash_dev_reg->data0 = new_frame_start_address;
    int command = (_getPageIndex(missing_page_index) << 8) + FLASHREAD;
    int res = SYSCALL(DOIO, &flash_dev_reg->command, command, NULL);
    if (res == FLASH_READ_ERROR) {
        // Trap
    }

    new_frame->sw_asid = new_asid;
    new_frame->sw_pageNo = missing_page_index;
    new_frame->sw_pte = &support_structure->sup_privatePgTbl[missing_page_index];

    setSTATUS(getSTATUS() & ~IECON); // Disabilita interrupt
    support_structure->sup_privatePgTbl[missing_page_index].pte_entryLO = support_structure->sup_privatePgTbl[missing_page_index].pte_entryLO | VALIDON;
    support_structure->sup_privatePgTbl[missing_page_index].pte_entryLO = support_structure->sup_privatePgTbl[missing_page_index].pte_entryLO & ~GETPAGENO + (missing_page_index << 12);
    TLBCLR(); // TODO Ottimizzare
    setSTATUS(getSTATUS() | IECON | IMON); // Abilita interrupt

    SYSCALL(VERHOGEN, (int)&swap_pool_sem, NULL, NULL);
    LDST(PREV_PROCESSOR_STATE);
}

/**
 * @brief Gestore delle eccezioni TLB.
*/
void TLBExceptionHandler() {
    support_t *support_structure = (support_t *)SYSCALL(GETSUPPORTPTR, NULL, NULL, NULL);
    
    switch (CAUSE_GET_EXCCODE(support_structure->sup_exceptState[0].cause)) {
        case TLBMOD:
            // Trap
            break;

        case TLBINVLDL:
        case TLBINVLDS:
            _TLBInvalidHandler(support_structure);
            break;
    }
}