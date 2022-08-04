#include "vmSupport.h"
#include <listx.h>
#include <initial.h>
#include <utilities.h>
#include <sysSupport.h>
#include <umps3/umps/cp0.h>
#include <umps3/umps/libumps.h>
#include <umps3/umps/types.h>
#include <umps3/umps/arch.h>

#define DATA_START_ADDR_OFFSET      0x18
#define DATA_FILE_SIZE_OFFSET       0x24
#define FRAME_POOL_START            *((int *)(KERNELSTACK+DATA_START_ADDR_OFFSET)) + *((int *)(KERNELSTACK+DATA_FILE_SIZE_OFFSET))

#define IS_FREE_FRAME(frame)        ((frame)->sw_asid == NOPROC)
#define FRAME_ADDRESS(index)        (FRAME_POOL_START + (index)*PAGESIZE)
#define FRAME_NUMBER(frame_addr)    (frame_addr - FRAME_POOL_START) / PAGESIZE

#define DISABLE_INTERRUPTS          setSTATUS(getSTATUS() & ~IECON)
#define ENABLE_INTERRUPTS           setSTATUS(getSTATUS() | IECON)

#define GET_VPN(entry_hi)           ((0b1 << 19) + ENTRYHI_GET_VPN(entry_hi)) // Inserisce il bit 1 implicito
#define INDEX_P_BIT_MASK            0x80000000

static semaphore_t swap_pool_sem;
static swap_t swap_pool_table[POOLSIZE];

/**
 * @brief Inizializza le strutture dati per la swap pool.
*/
void initSwapStructs() {
    swap_pool_sem.val = 1;
    swap_pool_sem.user_asid = NOPROC;

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
    return vpn % 0x80000;
}

/**
 * @brief Gestore TLB refill.
*/
void TLBRefillHandler() {
    // Estrazione entry nella page table
    int page_index = _getPageIndex(GET_VPN(PREV_PROCESSOR_STATE->entry_hi));
    pteEntry_t pt_entry = curr_process->p_supportStruct->sup_privatePgTbl[page_index];

    // Inserimento in TLB
    setENTRYHI(pt_entry.pte_entryHI);
    setENTRYLO(pt_entry.pte_entryLO);
    TLBWR();

    LDST(PREV_PROCESSOR_STATE);
}


/**
 * @brief Seleziona e restituisce un frame utilizzabile (non necessariamente vuoto).
 * @param frame_address Conterrà l'indirizzo del frame selezionato
 * @returns Entry della swap pool table del frame selezionato.
*/
static swap_t* _getFrame(memaddr *frame_address) {
    static int to_swap_frame_index = 0;
    swap_t *to_swap_frame_entry = NULL;
    memaddr to_swap_frame_address;

    // Muove il contatore ad un frame libero se esiste (altrimenti fa un giro completo e torna alla posizione originale)
    for (int i=0; i<POOLSIZE; i++) {
        if (IS_FREE_FRAME(&swap_pool_table[to_swap_frame_index])) {
            break;
        }
        to_swap_frame_index = (to_swap_frame_index+1) % POOLSIZE;
    }

    // Estrazione frame
    to_swap_frame_entry = &swap_pool_table[to_swap_frame_index];
    to_swap_frame_address = FRAME_ADDRESS(to_swap_frame_index);

    // Incremento del contatore (prossimo possibile frame da selezionare)
    to_swap_frame_index = (to_swap_frame_index+1) % POOLSIZE;

    *frame_address = to_swap_frame_address;
    return to_swap_frame_entry;
}

/**
 * Aggiorna (se esiste) una entry della page table nella TLB.
 * @param entry Entry della page table da aggiornare
*/
static void _updateTLB(pteEntry_t *entry) {
    // Ricerca frame nel TLB
    setENTRYHI(entry->pte_entryHI);
    TLBP();

    // Aggiornamento TLB
    if ((getINDEX() & INDEX_P_BIT_MASK) == 0) {
        setENTRYHI(entry->pte_entryHI);
        setENTRYLO(entry->pte_entryLO);
        TLBWI();
    }
}

/**
 * @brief Esegue un'operazione sul backing store
 * @param operation_command     Tipo di operazione (FLASHWRITE/FLASHREAD)
*/
static void _backingStoreOperation(int operation_command, int asid, int page_num, memaddr frame_address) {
    dtpreg_t *flash_dev_reg = (dtpreg_t *)DEV_REG_ADDR(4, asid-1);
    
    // Inizializzazione parametri
    flash_dev_reg->data0 = frame_address;
    int command = (page_num << 8) + operation_command;
    
    int status = SYSCALL(DOIO, (memaddr)&flash_dev_reg->command, command, 0);
    if (status == FLASH_WRITE_ERROR || status == FLASH_READ_ERROR) {
        trapExceptionHandler();
    }
}

/**
 * @brief Scrive sul flash drive corretto, i dati di una determinata pagina di un processo.
 * @param asid              ASID del processo
 * @param page_num          Numero della pagina da scrivere
 * @param frame_address     Indirizzo di inizio del frame che contiene la pagina
*/
static void _writePageToFlash(int asid, int page_num, memaddr frame_address) {
    _backingStoreOperation(FLASHWRITE, asid, page_num, frame_address);
}

/**
 * @brief Legge dal flash drive corretto, i dati di una determinata pagina di un processo.
 * @param asid              ASID del processo
 * @param page_num          Numero della pagina da scrivere
 * @param frame_address     Indirizzo di inizio del frame che conterrà la pagina
*/
static void _readPageFromFlash(int asid, int page_num, memaddr frame_address) {
    _backingStoreOperation(FLASHREAD, asid, page_num, frame_address);
}

/**
 * @brief Gestisce l'invalidazione di una pagina.
 * @param frame             Puntatore al descrittore del frame che contiene la pagina
 * @param frame_address     Indirizzo di inizio del frame che contiene la pagina
*/
static void _storePage(swap_t *frame, memaddr frame_address) {
    // Invalidazione della pagina
    DISABLE_INTERRUPTS;
    frame->sw_pte->pte_entryLO = frame->sw_pte->pte_entryLO & ~VALIDON;
    _updateTLB(frame->sw_pte);
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
    int page_num = _getPageIndex(GET_VPN(pt_entry->pte_entryHI));

    // Caricamento della nuova pagina in memoria
    _readPageFromFlash(asid, page_num, frame_address);

    // Aggiornamento descrittore del frame
    frame->sw_asid = asid;
    frame->sw_pageNo = page_num;
    frame->sw_pte = pt_entry;

    // Aggiornamento della page table
    DISABLE_INTERRUPTS;
    pt_entry->pte_entryLO = (pt_entry->pte_entryLO & ~ENTRYLO_PFN_MASK) | frame_address | VALIDON;
    _updateTLB(pt_entry);
    ENABLE_INTERRUPTS;
}

/**
 * @brief Gestore delle eccezioni TLB-invalid.
*/
static void _TLBInvalidHandler(support_t *support_structure) {
    P(&swap_pool_sem, support_structure->sup_asid);

    // Estrazione informazioni sulla pagina mancante
    int missing_page_index = _getPageIndex(GET_VPN(support_structure->sup_exceptState[PGFAULTEXCEPT].entry_hi));
    pteEntry_t *page_pt_entry = &support_structure->sup_privatePgTbl[missing_page_index];

    // Preparazione del frame da usare
    memaddr new_frame_address;
    swap_t *new_frame = _getFrame(&new_frame_address);

    // Manipolazione delle pagine
    if (!IS_FREE_FRAME(new_frame)) {
        _storePage(new_frame, new_frame_address);
    }
    _loadPage(page_pt_entry, new_frame, new_frame_address);

    V(&swap_pool_sem);
    LDST(&support_structure->sup_exceptState[PGFAULTEXCEPT]);
}

/**
 * @brief Gestore delle eccezioni TLB.
*/
void TLBExceptionHandler() {
    support_t *support_structure = (support_t *)SYSCALL(GETSUPPORTPTR, 0, 0, 0);
    
    switch (CAUSE_GET_EXCCODE(support_structure->sup_exceptState[PGFAULTEXCEPT].cause)) {
        case TLBMOD:
            trapExceptionHandler();
            break;

        case TLBINVLDL:
        case TLBINVLDS:
            _TLBInvalidHandler(support_structure);
            break;

        default:
            PANIC();
            break;
    }
}

/**
 * @brief Richiede il rilascio del semaforo della swap pool (se in possesso).
*/
void releaseSwapPoolSem() {
    support_t *support_structure = (support_t *)SYSCALL(GETSUPPORTPTR, 0, 0, 0);
    if (swap_pool_sem.user_asid == support_structure->sup_asid) {
        V(&swap_pool_sem);
    }
}

/**
 * @brief Libera i frame associati ad un ASID.
 * @param asid ASID del processo.
*/
void freeFrames(int asid) {
    P(&swap_pool_sem, asid);

    for (int i=0; i<POOLSIZE; i++) {
        if (swap_pool_table[i].sw_asid == asid) {
            swap_pool_table[i].sw_asid = NOPROC;
            swap_pool_table[i].sw_pageNo = 0;
            swap_pool_table[i].sw_pte = NULL;
        }
    }    

    V(&swap_pool_sem);
}

/**
 * @brief Inizializza la tabella delle pagine.
 * @param asid          ASID del processo.
 * @param page_table    Puntatore alla entry nella tabella delle pagine.
 * @param tmp_frame     Indirizzo di un frame di appoggio.
*/
void initPageTable(int asid, pteEntry_t *page_table, memaddr tmp_frame) {
    // Estrazione header
    _readPageFromFlash(asid, 0, tmp_frame);
    int *aout_header = (int *)tmp_frame;

    // Calcolo numero pagine .text
    int text_file_size = aout_header[5];
    int num_text_file = text_file_size / PAGESIZE;

    // Inizializzazione tabella delle pagine
    for (int i=0; i<31; i++) {
        page_table[i].pte_entryHI = ((0x80000+i) << ENTRYHI_VPN_BIT) + (asid << ENTRYHI_ASID_BIT);
        if (i < num_text_file) { page_table[i].pte_entryLO = 0; }
        else { page_table[i].pte_entryLO = 0 | ENTRYLO_DIRTY; }
    }
    // Pagina per lo stack
    page_table[31].pte_entryHI = ((0xBFFFF) << ENTRYHI_VPN_BIT) + (asid << ENTRYHI_ASID_BIT);
    page_table[31].pte_entryLO = 0 | ENTRYLO_DIRTY;
}