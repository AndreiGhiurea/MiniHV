#include "idt.h"
#include "native/memory.h"
#include "hv_heap.h"
#include "segment.h"
#include "data.h"

STATUS
CreateInterruptTable(
    IN      WORD           NumberOfEntries,
    OUT     IDT*           Idt
)
{
    WORD idtSize;

    // Idt->Limit is only on a WORD
    if( ( 0 == NumberOfEntries ) || ( NumberOfEntries * sizeof(IDT_ENTRY) > MAX_WORD  ) )
    {
        return STATUS_INVALID_PARAMETER1;
    }

    if( NULL == Idt )
    {
        return STATUS_INVALID_PARAMETER2;
    }

    idtSize = NumberOfEntries * sizeof( IDT_ENTRY );

    Idt->Base = HvAllocPoolWithTag( 0, idtSize, HEAP_IDT_TAG, 0 );
    if( NULL == Idt->Base )
    {
        return STATUS_HEAP_INSUFFICIENT_RESOURCES;
    }

    // the limit represents the last valid byte (not the size)
    Idt->Limit = idtSize - 1;

    return STATUS_SUCCESS;
}

STATUS
CreateInterruptDescriptor(
    IN_OPT  PVOID           EntryAddress,
    IN      BYTE            GateType,
    OUT     IDT_ENTRY*      Descriptor,
    IN      BOOLEAN         Present
)
{
    DWORD lowAddress;

    if( Present )
    {
        // we're talking about a real descriptor, we need to check the other fields
        if( NULL == EntryAddress )
        {
            return STATUS_INVALID_PARAMETER1;
        }

        if( ( TASK_GATE_ENTRY64 > GateType ) || ( TRAP_GATE_ENTRY64 < GateType ) )
        {
            // invalid descriptor
            return STATUS_INVALID_PARAMETER2;
        }
    }

    // this is the only field that needs validation in case of a dummy descriptor
    if( NULL == Descriptor )
    {
        return STATUS_INVALID_PARAMETER3;
    }

    lowAddress = QWORD_LOW( EntryAddress );

    // set all fields to 0
    memzero( Descriptor, sizeof( IDT_ENTRY ) );

    Descriptor->HighestDwordOffset = QWORD_HIGH( EntryAddress );
    Descriptor->HighWordOffset = DWORD_HIGH(lowAddress);
    Descriptor->Present = ( WORD ) Present;
    Descriptor->DPL = RING_ZERO_PL;
    Descriptor->Type = GateType;
    Descriptor->IST = gGlobalData.TaskData.IstUsed;
    Descriptor->SegmentSelector = CODE64_SEL;
    Descriptor->LowWordOffset = DWORD_LOW(lowAddress );

    return STATUS_SUCCESS;
}