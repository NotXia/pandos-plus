#include <umps3/umps/libumps.h>
#include <umps3/umps/cp0.h>
#include <pandos_types.h>
#include <sysSupport.h>
#include <vmSupport.h>

static state_t state[8];
static support_t support_arr[8];

/**
 * @brief Inizializzazione stato del processore di un processo.
 * @param asid ASID del processo.
 * @return Stato del processore per il processo.
 */
static void _createProcessorState(int asid, state_t *state) {
    state->reg_t9 = state->pc_epc = 0x800000B0;
    state->reg_sp = 0xC0000000;
    state->status = ALLOFF | (IMON | IEPON) | TEBITON | USERPON; // Interrupt abilitati + PLT abilitato + User mode
    state->entry_hi = (asid << ENTRYHI_ASID_BIT);
}

/**
 * @brief Inizializzazione support structure di un processo.
 * @param asid ASID del processo.
 * @return Support structure per il processo.
 */
static void _createSupportStructure(int asid, support_t *support) {
    support->sup_asid = asid;

    // Inizializzazione gestori eccezioni
    support->sup_exceptContext[PGFAULTEXCEPT].pc = (memaddr)TLBExceptionHandler;
    support->sup_exceptContext[PGFAULTEXCEPT].status = ALLOFF | (IMON | IEPON) | TEBITON; // Interrupt abilitati + PLT abilitato + Kernel mode
    support->sup_exceptContext[PGFAULTEXCEPT].stackPtr = (memaddr)&support->sup_stackTLB[499];
    support->sup_exceptContext[GENERALEXCEPT].pc = (memaddr)generalExceptionHandler;
    support->sup_exceptContext[GENERALEXCEPT].status = ALLOFF | (IMON | IEPON) | TEBITON; // Interrupt abilitati + PLT abilitato + Kernel mode
    support->sup_exceptContext[GENERALEXCEPT].stackPtr = (memaddr)&support->sup_stackGen[499];

    // Inizializzazione tabella delle pagine
    for (int i=0; i<31; i++) {
        support->sup_privatePgTbl[i].pte_entryHI = ((0x80000+i) << ENTRYHI_VPN_BIT) + (asid << ENTRYHI_ASID_BIT);
        support->sup_privatePgTbl[i].pte_entryLO = 0 | ENTRYLO_DIRTY;
    }
    support->sup_privatePgTbl[31].pte_entryHI = ((0xBFFFF) << ENTRYHI_VPN_BIT) + (asid << ENTRYHI_ASID_BIT);
    support->sup_privatePgTbl[31].pte_entryLO = 0 | ENTRYLO_DIRTY;
}

/**
 * @brief Inizializzazione processo.
 */
void _startProcess(int asid) {
    _createProcessorState(asid, &state[asid-1]);
    _createSupportStructure(asid, &support_arr[asid-1]);
    SYSCALL(CREATEPROCESS, (memaddr)&state[asid-1], PROCESS_PRIO_LOW, (memaddr)&support_arr[asid-1]);
}

/**
 * @brief Inizializzazione del sistema.
 */
void test() {
    int end_sem = 0;

    initSwapStructs();
    initSysStructs();

    for (int i=1; i<=8; i++) {
        _startProcess(i);
    }

    SYSCALL(PASSEREN, (memaddr)&end_sem, 0, 0);
}