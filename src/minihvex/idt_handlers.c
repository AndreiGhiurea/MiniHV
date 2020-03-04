#include "idt_handlers.h"
#include "hv_heap.h"
#include "log.h"
#include "segment.h"
#include "dmp_cpu.h"
#include "dmp_vmcs.h"
#include "vmcs.h"
#include "vmguest.h"
#include "data.h"

#define UNDEFINED_INTERRUPT_TEXT                "UNKNOWN INTERRUPT"

const char INTERRUPT_NAME[ExceptionVirtualizationException+1][MAX_PATH] = {   "#DE - Divide Error", "#DB - Debug Exception", "NMI Interrupt",
                                                                            "#BP - Breakpoint Exception", "#OF - Overflow Exception", "#BR - BOUND Range Exceeded Exception",
                                                                            "#UD - Invalid Opcode Exception", "#NM - Device Not Available Exception", "#DF - Double Fault Exception",
                                                                            "Coprocessor Segment Overrun", "#TS - Invalid TSS Exception", "#NP - Segment Not Present",
                                                                            "#SS - Stack Fault Exception", "#GP - General Protection Exception", "#PF - Page-Fault Exception",
                                                                            UNDEFINED_INTERRUPT_TEXT, "#MF - x87 FPU Floating-Point Error", "#AC - Allignment Check",
                                                                            "#MC - Machine-Check Exception", "#XM - SIMD Floating-Point Exception", "#VE - Virtualization Exception"
                                                                        };

extern void DivideError();
extern void DebugException();
extern void NMIInterrupt();
extern void BreakpointException();
extern void OverflowException();
extern void BoundRangeExceededException();
extern void InvalidOpcode();
extern void DeviceNotAvailable();
extern void DoubleFault();
extern void CoprocessorSegmentOverrun();
extern void InvalidTSS();
extern void SegmentNotPresent();
extern void StackFault();
extern void GeneralProtection();
extern void PageFault();
extern void FloatingPointX87Error();
extern void AlignmentCheck();
extern void MachineCheck();
extern void FloatingPointSIMD();
extern void VirtualizationException();

extern void SpuriousInterrupt();

//******************************************************************************
// Function:    CreateDummyDescriptors
// Description: Creates empty IDT descriptors.
// Returns:     void
// Parameter:   IDT_ENTRY * IdtEntries
// Parameter:   WORD NumberOfEntries
//******************************************************************************
static
void
CreateDummyDescriptors(
    OUT_WRITES_ALL(NumberOfEntries) IDT_ENTRY*  IdtEntries,
    IN                              WORD        NumberOfEntries
);

STATUS
InitializeIdtHandlers(
    OUT_PTR     PIDT*       Idt
)
{
    STATUS status;
    IDT* pIdt;

    if( NULL == Idt )
    {
        return STATUS_INVALID_PARAMETER1;
    }

    pIdt = NULL;
    pIdt = HvAllocPoolWithTag( PoolAllocateZeroMemory, sizeof( IDT ), HEAP_IDT_TAG, 0 );
    if( NULL == pIdt )
    {
        return STATUS_HEAP_INSUFFICIENT_RESOURCES;
    }

    // Allocates and initializes IDTR data
    status = CreateInterruptTable( NO_OF_IDT_ENTRIES_TO_CREATE, pIdt );
    if( !SUCCEEDED( status ) )
    {
        return status;
    }
    CreateDummyDescriptors( pIdt->Base, NO_OF_IDT_ENTRIES_TO_CREATE );


#pragma warning(push)
// warning C4152: nonstandard extension, function/data pointer conversion in expression
#pragma warning(disable:4152)
    // 00 - #DE - Divide Error
    status = CREATE_INTERRUPT_GATE_DESC(DivideError, &(pIdt->Base[ExceptionDivideError]) );
    ASSERT( SUCCEEDED( status ) );

    // 01 - #DB - Debug Exception
    status = CREATE_INTERRUPT_GATE_DESC(DebugException, &(pIdt->Base[ExceptionDebugException]) );
    ASSERT( SUCCEEDED( status ) );

    // 02 - NMI - NMI Interrupt
    status = CREATE_INTERRUPT_GATE_DESC(NMIInterrupt, &(pIdt->Base[ExceptionNMI]) );
    ASSERT( SUCCEEDED( status ) );

    // 03 - #BP - Breakpoint Exception
    status = CREATE_INTERRUPT_GATE_DESC(BreakpointException, &(pIdt->Base[ExceptionBreakpoint]) );
    ASSERT( SUCCEEDED( status ) );

    // 04 - #OF - Overflow Exception
    status = CREATE_INTERRUPT_GATE_DESC(OverflowException, &(pIdt->Base[ExceptionOverflow]) );
    ASSERT( SUCCEEDED( status ) );

    // 05 - #BR - BOUND Range Exceeded Exception
    status = CREATE_INTERRUPT_GATE_DESC(BoundRangeExceededException, &(pIdt->Base[ExceptionBoundRange]) );
    ASSERT( SUCCEEDED( status ) );

    // 06 - #UD - Invalid Opcode
    status = CREATE_INTERRUPT_GATE_DESC(InvalidOpcode, &(pIdt->Base[ExceptionInvalidOpcode]) );
    ASSERT( SUCCEEDED( status ) );

    // 07 - #NM - Device Not Available Exception
    status = CREATE_INTERRUPT_GATE_DESC( DeviceNotAvailable, &(pIdt->Base[ExceptionDeviceNotAvailable] ) );
    ASSERT( SUCCEEDED( status ) );

    // 08 - #DF - Double Fault
    status = CREATE_INTERRUPT_GATE_DESC(DoubleFault, &(pIdt->Base[ExceptionDoubleFault]) );
    ASSERT( SUCCEEDED( status ) );

    // 09 - COS - Coprocessor Segment Overrrun
    status = CREATE_INTERRUPT_GATE_DESC(CoprocessorSegmentOverrun, &(pIdt->Base[ExceptionCoprocOverrun]) );
    ASSERT( SUCCEEDED( status ) );

    // 10 - #TS - Invalid TSS Exception
    status = CREATE_INTERRUPT_GATE_DESC(InvalidTSS, &(pIdt->Base[ExceptionInvalidTSS]) );
    ASSERT( SUCCEEDED( status ) );

    // 11 - #NP - Segment Not Present
    status = CREATE_INTERRUPT_GATE_DESC(SegmentNotPresent, &(pIdt->Base[ExceptionSegmentNotPresent]) );
    ASSERT( SUCCEEDED( status ) );

    // 12 - #SS - Stack Fault Exception
    status = CREATE_INTERRUPT_GATE_DESC(StackFault, &(pIdt->Base[ExceptionStackFault]) );
    ASSERT( SUCCEEDED( status ) );

    // 13 - #GP - General Protection
    status = CREATE_INTERRUPT_GATE_DESC(GeneralProtection, &(pIdt->Base[ExceptionGeneralProtection]) );
    ASSERT( SUCCEEDED( status ) );

    // 14 - #PF - Page Fault
    status = CREATE_INTERRUPT_GATE_DESC(PageFault, &(pIdt->Base[ExceptionPageFault]) );
    ASSERT( SUCCEEDED( status ) );

    // 16 - #MF - x87 FPU Floating-Point Error
    status = CREATE_INTERRUPT_GATE_DESC(FloatingPointX87Error, &(pIdt->Base[ExceptionX87FpuException]) );
    ASSERT( SUCCEEDED( status ) );

    // 17 - #AC - Alignment Check Exception
    status = CREATE_INTERRUPT_GATE_DESC(AlignmentCheck, &(pIdt->Base[ExceptionAlignmentCheck]) );
    ASSERT( SUCCEEDED( status ) );

    // 18 - #MC - Machine Check Exception
    status = CREATE_INTERRUPT_GATE_DESC(MachineCheck, &(pIdt->Base[ExceptionMachineCheck]) );
    ASSERT( SUCCEEDED( status ) );

    // 19 - #XM - SIMD Floating-Point Exception
    status = CREATE_INTERRUPT_GATE_DESC(FloatingPointSIMD, &(pIdt->Base[ExceptionSIMDFpuException]) );
    ASSERT( SUCCEEDED( status ) );

    // 20 - #VE - Virtualization Exception
    status = CREATE_INTERRUPT_GATE_DESC(VirtualizationException, &(pIdt->Base[ExceptionVirtualizationException]) );
    ASSERT( SUCCEEDED( status ) );

    // 39 - Spurious Interrupt
    status = CREATE_INTERRUPT_GATE_DESC(SpuriousInterrupt, &(pIdt->Base[ExceptionApicSpuriousInterrupt]) );
    ASSERT( SUCCEEDED( status ) );


#pragma warning(pop)

    LOG( "Number of entries: %d\n", NO_OF_IDT_ENTRIES_TO_CREATE );
    LOG( "Idt limit: 0x%x\n", pIdt->Limit );
    LOG( "NO_OF_IDT_ENTRIES_TO_CREATE * sizeof( IDT_ENTRY ) - 1: %x\n", NO_OF_IDT_ENTRIES_TO_CREATE * sizeof( IDT_ENTRY ) - 1 );
    ASSERT( NULL != pIdt->Base );
    ASSERT( NO_OF_IDT_ENTRIES_TO_CREATE * sizeof( IDT_ENTRY ) - 1 == pIdt->Limit );

    // lidt Idt
    __lidt( pIdt );

    *Idt = pIdt;

    return status;
}

static
void
CreateDummyDescriptors(
    OUT_WRITES_ALL(NumberOfEntries) IDT_ENTRY*  IdtEntries,
    IN                              WORD        NumberOfEntries
)
{
    DWORD i;

    ASSERT( NULL != IdtEntries );
    ASSERT( 0 != NumberOfEntries );



    for( i = 0; i < NumberOfEntries; ++i )
    {
        CREATE_DUMMY_DESC(&(IdtEntries[i]));
    }
}

void
InterruptHandler(
    IN BYTE             InterruptIndex,
    IN PVOID            StackPointer,
    IN BOOLEAN          ErrorCodeAvailable,
    IN COMPLETE_PROCESSOR_STATE* ProcessorState
    )
{
    INTERRUPT_STACK_FORMAT* pStack;
    VCPU* vCpu;
    DWORD errorCode;
    BOOLEAN reinjectEventIfVmxOn;
    BOOLEAN handlingSucceeded;
    BOOLEAN injectOnGuestResume;
    BYTE eventType;
    BOOLEAN releasedLock;

    CHECK_STACK_ALIGNMENT;

    eventType = 0;
    pStack = NULL;
    reinjectEventIfVmxOn = FALSE;
    injectOnGuestResume = FALSE;
    handlingSucceeded = FALSE;
    releasedLock = FALSE;
    vCpu = GetCurrentVcpu();
    errorCode = 0;

    ASSERT_INFO( InterruptIndex <= ExceptionVirtualizationException, "We don't handle interrupt vector %d\n", InterruptIndex );
    ASSERT( NULL != StackPointer );

    // we may already hold the serial lock
    // in this case we will free it and reacquire it at the end
    // of the function
    if (LockIsOwner(&(gGlobalData.LogData.SerialLock)))
    {
        ReleaseLock(&gGlobalData.LogData.SerialLock);
        releasedLock = TRUE;
    }

    if( ErrorCodeAvailable )
    {
        errorCode = (DWORD) ((INTERRUPT_STACK_FORMAT_ERROR*)StackPointer)->ErrorCode;
        pStack = ( INTERRUPT_STACK_FORMAT* ) ( ( BYTE* ) StackPointer + sizeof( QWORD ) );
    }
    else
    {
        pStack = ( INTERRUPT_STACK_FORMAT* ) StackPointer;
    }

    switch( InterruptIndex )
    {
    case ExceptionNMI:
        // EventType = InterruptionTypeNMI,
        // If VMX is on we will inject the event to guest
        // We will inject event to guest only after we finished what we were doing before (after IRET)
        eventType = InterruptionTypeNMI;
        reinjectEventIfVmxOn = TRUE;
        injectOnGuestResume = TRUE;
        break;
    case ExceptionPageFault:
        {
            PVOID faultAddress;
            PVOID pStackGuardBottom;
            PVOID pStackGuardTop;

            faultAddress = ( PVOID )__readcr2();
            LOGP( "Faulting Address: 0x%X\n", faultAddress );

            // check to see if we didn't use more of our stack then we actually had available :)
            pStackGuardBottom = (PVOID) ((QWORD)GetCurrentPcpu()->StackBase - EFFECTIVE_STACK_SIZE);
            pStackGuardTop = (PVOID)((QWORD)pStackGuardBottom + (STACK_SIZE - EFFECTIVE_STACK_SIZE));

            LOGP("GetCurrentPcpu()->StackBase: 0x%X\n", GetCurrentPcpu()->StackBase);

            // this must be a strict comparison because if it's equal to pStackGuardTop the address
            // is still valid
            if (faultAddress >= pStackGuardBottom && faultAddress < pStackGuardTop)
            {
                ASSERT_INFO(FALSE, "Recursive execution reached! BYE, BYE!");
            }

            if (vCpu->EnteredVMX)
            {
                DumpCurrentVmcs(&GetCurrentVcpu()->ProcessorState->RegisterArea);
            }
        }
        break;
    case ExceptionBreakpoint:
    case ExceptionOverflow:
        // see 24.8.3 for details: #BP and #OF are software exceptions
        eventType = InterruptionTypeSoftwareException;
        break;
    default:
        // EventType = InterruptionTypeHardwareException
        // If VMX is on we will inject the event to guest
        // We will inject event into guest NOW (don't wait for IRET)
        eventType = InterruptionTypeHardwareException;
        reinjectEventIfVmxOn = TRUE;
        break;
    }


    if( vCpu->EnteredVMX && reinjectEventIfVmxOn )
    {
        // we will reinject the exception, let the guest handle it :)
        STATUS status;
        DWORD* pErrorCode;


        if( ErrorCodeAvailable )
        {
            pErrorCode = &errorCode;
        }
        else
        {
            pErrorCode = NULL;
        }

        status = GuestInjectEvent(InterruptIndex, eventType, pErrorCode);
        ASSERT_INFO(SUCCEEDED(status), "GuestInjectEvent failed with status: 0x%x\n", status);

        if (releasedLock)
        {
            AcquireLock(&gGlobalData.LogData.SerialLock);
            releasedLock = FALSE;
        }

        if( !injectOnGuestResume )
        {
            // this function never returns
            VmxResumeGuest();
        }

        handlingSucceeded = TRUE;
    }

    if( !handlingSucceeded )
    {
        LOGP("Interrupt: 0x%x, %s\n", InterruptIndex, INTERRUPT_NAME[InterruptIndex]);
        LOGP("RIP: 0x%X\n", pStack->Rip);
        LOGP("CS: 0x%X\n", pStack->CS);
        LOGP("RFLAGS: 0x%X\n", pStack->RFLAGS);
        LOGP("RSP: 0x%X\n", pStack->Rsp);
        LOGP("SS: 0x%X\n", pStack->SS);
        if (ErrorCodeAvailable)
        {
            LOGP( "Error code: 0x%x\n", errorCode );
        }


        ASSERT_INFO( FALSE, "Couldn't handle exception\n" );
    }
}