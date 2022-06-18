#include "vmSupport.h"
#include <pandos_types.h>
#include <listx.h>
#include <initial.h>
#include <umps3/umps/cp0.h>
#include <umps3/umps/libumps.h>
#include <umps3/umps/types.h>

#define IS_FREE_FRAME(frame)        frame->sw_asid == NOPROC
#define FRAME_ADDRESS(index)        (FRAMEPOOLSTART + (index)*PAGESIZE)
#define FRAME_NUMBER(frame_addr)    (frame_addr - FRAMEPOOLSTART) % PAGESIZE
#define DISABLE_INTERRUPTS          setSTATUS(getSTATUS() & ~IECON)
#define ENABLE_INTERRUPTS           setSTATUS(getSTATUS() | IECON | IMON)

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
static swap_t* _getFrame(memaddr *frame_address) {
    swap_t *to_swap_frame = &swap_pool_table[swap_pool_index];
    memaddr to_swap_frame_address = FRAME_ADDRESS(swap_pool_index);

    swap_pool_index++;
    if (swap_pool_index >= POOLSIZE) { swap_pool_index = 0; }

    *frame_address = to_swap_frame_address;
    return to_swap_frame;
}

/**
 * @brief Scrive sul flash drive corretto, i dati di una determinata pagina di un processo.
 * @param asid              ASID del processo
 * @param page_num          Numero della pagina da scrivere
 * @param frame_address     Indirizzo di inizio del frame che contiene la pagina
*/
static void _writePageToFlash(int asid, int page_num, memaddr frame_address) {
    dtpreg_t *flash_dev_reg = (dtpreg_t *)DEV_REG_ADDR(4, asid);
    
    flash_dev_reg->data0 = frame_address;
    int command = (page_num << 8) + FLASHWRITE;
    
    int res = SYSCALL(DOIO, &flash_dev_reg->command, command, NULL);
    if (res == FLASH_WRITE_ERROR) {
        // Trap
    }
}

/**
 * @brief Legge dal flash drive corretto, i dati di una determinata pagina di un processo.
 * @param asid              ASID del processo
 * @param page_num          Numero della pagina da scrivere
 * @param frame_address     Indirizzo di inizio del frame che conterrà la pagina
*/
static void _readPageFromFlash(int asid, int page_num, memaddr frame_address) {
    dtpreg_t *flash_dev_reg = (dtpreg_t *)DEV_REG_ADDR(4, asid);

    flash_dev_reg->data0 = frame_address;
    int command = (page_num << 8) + FLASHREAD;

    int res = SYSCALL(DOIO, &flash_dev_reg->command, command, NULL);
    if (res == FLASH_READ_ERROR) {
        // Trap
    }
}

/**
 * @brief Gestisce l'invalidazione di una pagina.
 * @param frame             Puntatore al descrittore del frame che contiene la pagina
 * @param frame_address     Indirizzo di inizio del frame che contiene la pagina
*/
static void _storePage(swap_t *frame, memaddr frame_address) {
    DISABLE_INTERRUPTS;
    // Invalidazione della pagina
    frame->sw_pte->pte_entryLO = frame->sw_pte->pte_entryLO & ~VALIDON;
    TLBCLR();
    ENABLE_INTERRUPTS;

    // Salvataggio della vecchia pagina nel flash drive del processo
    _writePageToFlash(frame->sw_asid, frame->sw_pageNo, frame_address);
}

/**
 * @brief Gestisce il caricamento di una pagina in memoria.
 * @param pt_entry          Puntatore alla riga nella page table della pagina da caricare
 * @param frame             Puntatore al descrittore del frame che conterrà la pagina
 * @param frame_address     Indirizzo di inizio del frame che conterrà la pagina
*/
static void _loadPage(pteEntry_t *pt_entry, swap_t *frame, memaddr frame_address) {
    int asid = ENTRYHI_GET_ASID(pt_entry->pte_entryHI);
    int page_num = _getPageIndex(ENTRYHI_GET_VPN(pt_entry->pte_entryHI));

    // Caricamento della nuova pagina in memoria
    _readPageFromFlash(asid, page_num, frame_address);

    // Aggiornamento descrittore del frame
    frame->sw_asid = asid;
    frame->sw_pageNo = page_num;
    frame->sw_pte = pt_entry;

    // Aggiornamento della page table
    DISABLE_INTERRUPTS;
    pt_entry->pte_entryLO = pt_entry->pte_entryLO | VALIDON; // Valid bit
    pt_entry->pte_entryLO = (pt_entry->pte_entryLO & ~ENTRYLO_PFN_MASK) + (FRAME_NUMBER(frame_address) << 12); // PFN
    TLBCLR(); // TODO Ottimizzare
    ENABLE_INTERRUPTS;
}

/**
 * @brief Gestore delle eccezioni TLB-invalid.
*/
static void _TLBInvalidHandler(support_t *support_structure) {
    SYSCALL(PASSEREN, (int)&swap_pool_sem, NULL, NULL);

    // Estrazione informazioni sulla pagina mancante
    int missing_page_index = _getPageIndex(ENTRYHI_GET_VPN(PREV_PROCESSOR_STATE->entry_hi));
    pteEntry_t *page_pt_entry = &support_structure->sup_privatePgTbl[missing_page_index];

    // Preparazione del frame da usare
    memaddr new_frame_address;
    swap_t *new_frame = _getFrame(&new_frame_address);

    // Manipolazione delle pagine
    if (!IS_FREE_FRAME(new_frame)) { // TODO Controllare dirty bit
        _storePage(new_frame, new_frame_address);
    }
    _loadPage(page_pt_entry, new_frame, new_frame_address);

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