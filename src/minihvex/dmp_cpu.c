#include "dmp_cpu.h"
#include "vmguest.h"
#include "log.h"
#include "data.h"

#define MAX_REGISTER_NAME_LENGTH        3

// +1 because of NULL terminator
const char REGISTER_NAMES[RegisterR15+1][MAX_REGISTER_NAME_LENGTH+1] = {    "RAX", "RCX", "RDX", "RBX", "RSP", "RBP", "RSI", "RDI",
                                                                            "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15" };

//******************************************************************************
// Function:      RetrieveRegisterName
// Description: Returns the name of the register.
// Returns:       const char* - Name of the register, NULL if index out of bounds.
// Parameter:     GeneralPurposeRegisterIndexes Index
//******************************************************************************
__forceinline
const
char*
_RetrieveRegisterName(
    IN      GeneralPurposeRegisterIndexes     Index
)
{
    if( Index > RegisterR15 )
    {
        return NULL;
    }

    return REGISTER_NAMES[Index];
}

STATUS
DumpPhysicalCpu(
    IN  LIST_ENTRY*     CpuEntry,
    IN  PVOID           Context
    )
{
    PCPU* pCpu;

    LOGL( "Entering %s\n", __FUNCTION__ );

    UNREFERENCED_PARAMETER( Context );

    if( NULL == CpuEntry )
    {
        return STATUS_INVALID_PARAMETER1;
    }

    pCpu = CONTAINING_RECORD( CpuEntry, PCPU, ListEntry );

    LOG( "APIC ID: %d\n", pCpu->ApicID );
    LOG( "Stack Base: 0x%X\n", pCpu->StackBase );

    return STATUS_SUCCESS;
}

void
DumpRegisterArea(
    IN  REGISTER_AREA*              ProcessorState
    )
{
    ASSERT(ProcessorState != NULL);

    for (DWORD i = 0; i <= RegisterR15; ++i)
    {
        QWORD regValue = ProcessorState->RegisterValues[i];

        LOG("%s: 0x%X\n", _RetrieveRegisterName(i), regValue);
    }

    LOG("RIP: 0x%X\n", ProcessorState->Rip);
    LOG("Rflags: 0x%X\n", ProcessorState->Rflags);

}

STATUS
DumpProcessorState(
    IN  COMPLETE_PROCESSOR_STATE*    ProcessorState
    )
{
    if (NULL == ProcessorState)
    {
        return STATUS_INVALID_PARAMETER1;
    }


    LOG("\nProcessor State:\n");


    DumpRegisterArea(&ProcessorState->RegisterArea);

    return STATUS_SUCCESS;
}