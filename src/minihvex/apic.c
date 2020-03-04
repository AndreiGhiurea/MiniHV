#include "apic.h"
#include "display.h"
#include "log.h"
#include "cpumu.h"
#include "pit.h"
#include "paging_tables.h"
#include "dmp_cpu.h"
#include "hv_heap.h"
#include "idt.h"
#include "idt_handlers.h"
#include "dmp_memory.h"
#include "data.h"

//******************************************************************************
// Function:    CpuInitializationSequence
// Description: This is called for each physical CPU and issues the
//              INIT-SIPI-SIPI protocol and then waits for initialization to
//              terminate.
// Returns:     STATUS
// Parameter:   IN PLIST_ENTRY ListEntry
// Parameter:   IN PVOID FunctionContext - UNUSED
//******************************************************************************
static
STATUS
_ApicCpuInitializationSequence(
    IN  PLIST_ENTRY ListEntry,
    IN  PVOID       FunctionContext
    );

STATUS
ApicMapRegister(
    void
    )
{
    QWORD apicBaseRegister;
    PVOID localApicBaseAddress;

    apicBaseRegister = 0;
    localApicBaseAddress = NULL;

    // we can't initialize the APIC twice
    ASSERT( NULL == gGlobalData.ApicData.ApicBaseAddress );

    apicBaseRegister = __readmsr( IA32_APIC_BASE_MSR );

    // we must be the BSP
    ASSERT( apicBaseRegister & IA32_APIC_BSP_FLAG );

    localApicBaseAddress = ( PVOID ) IA32_APIC_BASE_MASK(apicBaseRegister);

    // make sure we map APIC in UC memory
    localApicBaseAddress = MapMemoryInvalidate( localApicBaseAddress, PAGE_SIZE, MEMORY_TYPE_STRONG_UNCACHEABLE, TRUE );
    if( NULL == localApicBaseAddress )
    {
        return STATUS_MEMORY_CANNOT_BE_MAPPED;
    }

    // we initialize the Apic base address
    gGlobalData.ApicData.ApicBaseAddress = localApicBaseAddress;

    return STATUS_SUCCESS;
}

void
ApicWakeUpCpus(
    IN     PLIST_ENTRY     PhysicalCpuList
    )
{
    PCPU* pCpu;
    STATUS status;

    ASSERT( NULL != PhysicalCpuList );
    ASSERT( NULL != gGlobalData.ApicData.ApicBaseAddress );

    status = STATUS_SUCCESS;

    pCpu = NULL;

    // we have the BSP
    gGlobalData.ApicData.ActiveCpus = 1;

    EvtInitialize( &gGlobalData.ApicData.WakeupLock, EventTypeSynchronization, FALSE );

    status = ForEachElementExecute( PhysicalCpuList, _ApicCpuInitializationSequence, NULL, TRUE );
    ASSERT( SUCCEEDED( status ) );

    printf( "[%d]Hello from CPU 0\n", CpuGetApicId() );

#ifdef TEST
    TestCommunications( TEST_COMM_NO_OF_TRIALS );
#endif
}

static
STATUS
_ApicCpuInitializationSequence(
    IN  PLIST_ENTRY ListEntry,
    IN  PVOID       FunctionContext
    )
{
    PCPU* pCurEntry;
    DWORD currentNumberOfActiveCpus;
    BYTE apicId;
    STATUS status;

    ASSERT( NULL != gGlobalData.ApicData.ApicBaseAddress );

    if( NULL == ListEntry )
    {
        return STATUS_INVALID_PARAMETER1;
    }

    status = STATUS_SUCCESS;
    pCurEntry = CONTAINING_RECORD( ListEntry, PCPU, ListEntry );

    apicId = pCurEntry->ApicID;

    if( pCurEntry->BspProcessor )
    {
        // BSP
        __changeStack( gGlobalData.MiniHvInformation.InitialStackBase, pCurEntry->StackBase );

        // see if we changed the stack properly :)
        CHECK_STACK_ALIGNMENT;

        status = VmxCheckAndSetupCpu();

        ASSERT( SUCCEEDED( status ) );

        LOG( "About to enter VmxStartVmxOn\n" );
        status = VmxStartVmxOn();

        ASSERT( SUCCEEDED( status ) );

        // here we need to call VmxPrepareStructures
        // this will prepare the VMCS that will get copied to each CPU
        // NOTE: This can only be done after VMXON
        status = VmxSetupVmcsStructures( &gGlobalData.VmxConfigurationData );

        ASSERT( SUCCEEDED( status ) );

        return STATUS_SUCCESS;
    }

    currentNumberOfActiveCpus = _InterlockedAnd( &( gGlobalData.ApicData.ActiveCpus ), MAX_DWORD );

    LOG( "About to wake up CPU: %d\n", apicId );

    ApicSendIpi( apicId, ApicDeliveryModeINIT, ApicDestinationShorthandNone, NULL);
    PitSleep(INIT_SLEEP);

    ApicSendIpi( apicId, ApicDeliveryModeSIPI, ApicDestinationShorthandNone, &gGlobalData.MiniHvInformation.TrampolineSipiVector);
    PitSleep(SIPI_SLEEP);

    ApicSendIpi( apicId, ApicDeliveryModeSIPI, ApicDestinationShorthandNone, &gGlobalData.MiniHvInformation.TrampolineSipiVector);
    PitSleep(SIPI_SLEEP);

    // wait for thread to wake up
    EvtWaitForSignal(&gGlobalData.ApicData.WakeupLock);

    ASSERT( (DWORD) _InterlockedAnd( &( gGlobalData.ApicData.ActiveCpus ), MAX_DWORD ) == currentNumberOfActiveCpus + 1 );

    LOG( "Finished waking up CPU: %d\n", apicId );

    return STATUS_SUCCESS;
}

void
ApicInitAPCpu(
    void
    )
{
    DWORD apicId;
    LIST_ENTRY* pCurEntry;
    PCPU* pCpu;
    BOOLEAN found;
    STATUS status;

    CHECK_STACK_ALIGNMENT;

    apicId = CpuGetApicId();
    found = FALSE;
    status = STATUS_SUCCESS;

    ASSERT( !IsListEmpty( &gGlobalData.ApicData.PhysCpuListHead ) );

    CpuMuEnableFeatures();

    // we need to find our entry from the processor list
    // so we can change to the new stack
    pCurEntry = gGlobalData.ApicData.PhysCpuListHead.Flink;
    do
    {
        pCpu = CONTAINING_RECORD( pCurEntry, PCPU, ListEntry );

        if( pCpu->ApicID == apicId )
        {
            found = TRUE;
            break;
        }
        pCurEntry = pCurEntry->Flink;
    } while ( pCurEntry != &(gGlobalData.ApicData.PhysCpuListHead ) );

    ASSERT( found );

    // set up new stack
    __changeStack( gGlobalData.MiniHvInformation.InitialStackBase, pCpu->StackBase );

    // check to see if we changed the stack properly
    CHECK_STACK_ALIGNMENT;

    // set up GS and FS
    __writemsr(IA32_GS_BASE_MSR, pCpu);
    __writemsr(IA32_FS_BASE_MSR, pCpu->VirtualCpu);

    status = CpuCreateTSSDescriptor();
    ASSERT( SUCCEEDED( status ) );

    // load TR
    __loadTR( pCpu->TrSelector );

    // load the IDT
    __lidt( gGlobalData.Idt );

    ASSERT( GetCurrentPcpu() == pCpu );
    ASSERT( GetCurrentVcpu() == pCpu->VirtualCpu );

    // increment the number of active CPUs
    _InterlockedIncrement( &gGlobalData.ApicData.ActiveCpus );

    // signal BSP
    EvtSignal( &gGlobalData.ApicData.WakeupLock );

    // say hello
    // -1 because any numbering starts from 0 :)
    printf("[%d]Hello from CPU %d\n", apicId, gGlobalData.ApicData.ActiveCpus - 1);
    LOGPL("Hello\n");

    status = VmxCheckAndSetupCpu();

    ASSERT(SUCCEEDED(status));

    status = VmxStartVmxOn();

    ASSERT(SUCCEEDED(status));

    status = VmxSetupVmcsStructures(&(gGlobalData.VmxConfigurationData));

    ASSERT(SUCCEEDED(status));

    status = VmxStartGuest();

    ASSERT(SUCCEEDED(status));

#if defined(TEST) && defined(MULTI_TEST)
    TestCommunications( TEST_COMM_NO_OF_TRIALS );
    TestHeapFunctions();
#endif
}

void
ApicSendIpi(
    IN      BYTE                            ApicId,
    IN      APIC_DELIVERY_MODE              DeliveryMode,
    IN      APIC_DESTINATION_SHORTHAND      DestinationShorthand,
    IN_OPT  BYTE*                           Vector
)
{
    volatile ICR_LOW_REGISTER* pIcrLow;     // InterruptControlRegister ( LOW DWORD )
    volatile ICR_HIGH_REGISTER* pIcrHigh;    // InterruptControlRegister ( HIGH DWORD )
    ICR_LOW_REGISTER lowIcrValue = { 0 };
    ICR_HIGH_REGISTER highIcrValue = { 0 };

    ASSERT(NULL != gGlobalData.ApicData.ApicBaseAddress);

    if( ( ApicDestinationShorthandSelf == DestinationShorthand ) || ( ApicDestinationShorthandAll == DestinationShorthand ) )
    {
        // in this case only FIXED IPI can be sent
        ASSERT( ApicDeliveryModeFixed == DeliveryMode );
    }

    // mov  ESI,    ICR_LOW
    pIcrLow = ( ICR_LOW_REGISTER* ) ( ( BYTE* ) gGlobalData.ApicData.ApicBaseAddress + APIC_ICR_LOW_REG_OFFSET );
    pIcrHigh = ( ICR_HIGH_REGISTER* ) ( ( BYTE* ) gGlobalData.ApicData.ApicBaseAddress + APIC_ICR_HIGH_REG_OFFSET );
    highIcrValue.Destination = ApicId;

    if( NULL != Vector )
    {
        lowIcrValue.Vector = *Vector;
    }

    lowIcrValue.DeliveryMode = DeliveryMode;

    // ASSERT
    lowIcrValue.Level = 1;

    lowIcrValue.DestinationShorthand = DestinationShorthand;

    *pIcrHigh = highIcrValue;
    *pIcrLow = lowIcrValue;
}