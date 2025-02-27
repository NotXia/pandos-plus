#include <umps3/umps/libumps.h>
#include <umps3/umps/cp0.h>
#include <pandos_types.h>
#include <sysSupport.h>
#include <vmSupport.h>

static support_t support_arr[UPROCMAX];
static struct list_head free_support;
static int master_sem;


/**
 * @brief Alloca una support structure.
 * @return Support structure pronta all'uso.
 */
static support_t *_allocate() {
    // Rimozione in testa
    support_t *support_structure = container_of(free_support.next, support_t, p_list);
    list_del(free_support.next);

    return support_structure;
}

/**
 * @brief Dealloca una support structure.
 * @param support_structure Puntatore alla support structure da deallocare.
 */
static void _deallocate(support_t *support_structure) {
    list_add(&support_structure->p_list, &free_support);
}

/**
 * @brief Inizializza lo stack dei support structure.
 * @param stack Puntatore alla testa dello stack.
 */
static void _initFreeSupportStack() {
    INIT_LIST_HEAD(&free_support);

    for (int i=0; i<UPROCMAX; i++) {
        _deallocate(&support_arr[i]);
    }
}

/**
 * @brief Restituisce l'indirizzo di una pagina libera che può essere usata come stack.
 * @return L'indirizzo della pagina adibita a stack.
*/
static memaddr _getStackFrame() {
    static int curr_offset = 0;
    memaddr ram_top;
    RAMTOP(ram_top);

    memaddr frame_address = (ram_top-PAGESIZE) - (curr_offset*PAGESIZE);
    curr_offset++;

    return frame_address;
}

/**
 * @brief Inizializzazione stato del processore di un processo.
 * @param asid ASID del processo.
 * @param state Conterrà lo stato del processore per il processo.
 */
static void _initProcessorState(int asid, state_t *state) {
    state->reg_t9 = state->pc_epc = 0x800000B0;
    state->reg_sp = 0xC0000000;
    state->status = ALLOFF | (IMON | IEPON) | TEBITON | USERPON; // Interrupt abilitati + PLT abilitato + User mode
    state->entry_hi = (asid << ENTRYHI_ASID_BIT);
}

/**
 * @brief Inizializzazione support structure di un processo.
 * @param asid ASID del processo.
 * @param support Conterrà la support structure inizializzata per il processo.
 */
static void _initSupportStructure(int asid, support_t *support) {
    support->sup_asid = asid;

    // Inizializzazione gestori eccezioni
    memaddr page_fault_stack = _getStackFrame();
    memaddr general_stack = _getStackFrame();
    support->sup_exceptContext[PGFAULTEXCEPT].pc = (memaddr)TLBExceptionHandler;
    support->sup_exceptContext[PGFAULTEXCEPT].status = ALLOFF | (IMON | IEPON) | TEBITON; // Interrupt abilitati + PLT abilitato + Kernel mode
    support->sup_exceptContext[PGFAULTEXCEPT].stackPtr = page_fault_stack;
    support->sup_exceptContext[GENERALEXCEPT].pc = (memaddr)generalExceptionHandler;
    support->sup_exceptContext[GENERALEXCEPT].status = ALLOFF | (IMON | IEPON) | TEBITON; // Interrupt abilitati + PLT abilitato + Kernel mode
    support->sup_exceptContext[GENERALEXCEPT].stackPtr = general_stack;

    /*
        Inizializzazione della tabella delle pagine.
        Dal momento che il processo non è ancora stato avviato, si può usare come frame temporaneo uno di quelli assegnati per lo stack.
        Lo stack viene inizializzato puntando alla fine del frame. Questo perché lo stack cresce verso il basso (nel senso inverso alla memoria).
        Bisogna quindi calcolare l'indirizzo "dell'altra estremità" del frame (l'inizio) per poterlo usare correttamente.
    */
    memaddr tmp_frame = general_stack - PAGESIZE + WORDLEN;
    initPageTable(asid, support->sup_privatePgTbl, tmp_frame);
}

/**
 * @brief Inizializzazione e avvio di un processo.
 * @param asid ASID del processo da inizializzare.
 */
static void _startProcess(int asid) {
    state_t state;
    support_t *support_structure = _allocate();
    
    _initProcessorState(asid, &state);
    _initSupportStructure(asid, support_structure);
    SYSCALL(CREATEPROCESS, (memaddr)&state, PROCESS_PRIO_LOW, (memaddr)support_structure);
}

/**
 * @brief Gestisce il rilascio di alcune risorse a seguito della terminazione di un processo.
 */
void signalProcessTermination() {
    _deallocate( (support_t *)SYSCALL(GETSUPPORTPTR, 0, 0, 0) );
    SYSCALL(VERHOGEN, (memaddr)&master_sem, 0, 0);
}

/**
 * @brief Inizializzazione del sistema.
 */
void test() {
    master_sem = 0;

    initSwapStructs();
    initSysStructs();
    _initFreeSupportStack();

    // Initializza gli uproc
    for (int i=1; i<=UPROCMAX; i++) {
        _startProcess(i);
    }

    // Attende la terminazione di tutti gli uproc
    for (int i=0; i<UPROCMAX; i++) {
        SYSCALL(PASSEREN, (memaddr)&master_sem, 0, 0);
    }

    SYSCALL(TERMPROCESS, 0, 0, 0);
}