#include <umps3/umps/libumps.h>
#include <umps3/umps/cp0.h>
#include <pandos_types.h>
#include <sysSupport.h>
#include <vmSupport.h>

/**
 * @brief Inizializzazione stato del processore di un processo.
 * @param asid ASID del processo.
 * @return Stato del processore per il processo.
 */
static state_t _createProcessorState(int asid) {
    state_t state;

    state.reg_t9 = state.pc_epc = 0x800000B0;
    state.reg_sp = 0xC0000000;
    state.status = ALLOFF | (IMON | IEPON) | TEBITON | USERPON; // Interrupt abilitati + PLT abilitato + User mode
    state.entry_hi = asid << ENTRYHI_ASID_BIT;

    return state;
}

/**
 * @brief Inizializzazione support structure di un processo.
 * @param asid ASID del processo.
 * @return Support structure per il processo.
 */
static support_t _createSupportStructure(int asid) {
    support_t support;

    support.sup_asid = asid;

    // Inizializzazione gestori eccezioni
    support.sup_exceptContext[PGFAULTEXCEPT].pc = (memaddr)TLBExceptionHandler;
    support.sup_exceptContext[PGFAULTEXCEPT].status = ALLOFF | (IMON | IEPON) | TEBITON; // Interrupt abilitati + PLT abilitato + Kernel mode
    support.sup_exceptContext[PGFAULTEXCEPT].stackPtr = &support.sup_stackTLB[499];
    support.sup_exceptContext[GENERALEXCEPT].pc = (memaddr)generalExceptionHandler;
    support.sup_exceptContext[GENERALEXCEPT].status = ALLOFF | (IMON | IEPON) | TEBITON; // Interrupt abilitati + PLT abilitato + Kernel mode
    support.sup_exceptContext[PGFAULTEXCEPT].stackPtr = &support.sup_stackGen[499];

    // Inizializzazione tabella delle pagine
    for (int i=0; i<31; i++) {
        support.sup_privatePgTbl[i].pte_entryHI = ((0x80000+i) << ENTRYHI_VPN_BIT) + (asid << ENTRYHI_ASID_BIT);
        support.sup_privatePgTbl[i].pte_entryLO = 0 | ENTRYLO_DIRTY | ENTRYLO_GLOBAL;
    }
    support.sup_privatePgTbl[31].pte_entryHI = ((0xBFFFF) << ENTRYHI_VPN_BIT) + (asid << ENTRYHI_ASID_BIT);
    support.sup_privatePgTbl[31].pte_entryLO = 0 | ENTRYLO_DIRTY | ENTRYLO_GLOBAL;

    return support;
}

/**
 * @brief Inizializzazione processo.
 */
void _startProcess(int asid) {
    state_t state = _createProcessorState(asid);
    support_t support = _createSupportStructure(asid);
    SYSCALL(CREATEPROCESS, (memaddr)&state, PROCESS_PRIO_LOW, (memaddr)&support);
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

    SYSCALL(PASSEREN, &end_sem, NULL, NULL);
}