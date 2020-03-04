#include "vmmemory.h"
#include "log.h"
#include "data.h"
#include "vmcs.h"
#include "paging_tables.h"
#include "native/memory.h"
#include "idt_handlers.h"
#include "dmp_vmcs.h"
#include "dmp_memory.h"
#include "hv_heap.h"

#pragma warning(push)

// warning C28039: The type of actual parameter '24576|2048|((0<<1))|0' should exactly match the type 'VMCS_FIELD':
#pragma warning(disable: 28039)
#define IVT_15H_ENTRY           0x15

#define IVT_NEW_SELECTOR        0x0
#define IVT_NEW_OFFSET          0x360
#define IVT_NEW_ADDRESS         ((IVT_NEW_SELECTOR<<4)+IVT_NEW_OFFSET)

#define NO_OF_BYTES_TO_PATCH    14

#define OFFSET_TO_OFFSET        6
#define OFFSET_TO_SELECTOR      8
#define OFFSET_TO_VMCALL        10

//  cmp ax, 0xE820              0x3D 0x20 0xE8
//  je  $+5                     0x74 0x05
//  jmp oldSel:oldOff           0xEA $oldOff:$oldSel
// 3 + 2 + 5 + 3 + 1 = 14
// +6 to offset, +8 to selector

BYTE PATCH_CODE[NO_OF_BYTES_TO_PATCH] = {       0x3D, 0x20, 0xE8,                   // cmp AX, 0xE820
                                                0x74, 0x05,                         // je $+5
                                                0xEA, 0xCC, 0xCC, 0xCC, 0xCC,       // JMP oldSel:oldOfset
                                                0x0F, 0x01, 0xC1,                   // VMCALL - JE is HERE
                                                0xCF                                // IRET
                                                };
STATUS
PatchMemoryMap(
    INOUT   INT15_MEMORY_MAP*       MemoryMap,
    IN      QWORD                   BytesToCut,
    IN      QWORD                   StartAddress
    )
{
    STATUS status;
    INT15_MEMORY_MAP_LIST_ENTRY* pMap;
    INT15_MEMORY_MAP_LIST_ENTRY* pNewEntry;
    INT15_MEMORY_MAP_LIST_ENTRY* pReservedEntry;
    LIST_ENTRY* pCurEntry;
    QWORD endAddress;
    BOOLEAN found;


    status = STATUS_INSUFFICIENT_MEMORY;
    pCurEntry = NULL;
    pNewEntry = NULL;
    pReservedEntry = NULL;
    found = FALSE;
    pMap = NULL;

    if( NULL == MemoryMap )
    {
        return STATUS_INVALID_PARAMETER1;
    }

    if( 0 == BytesToCut )
    {
        return STATUS_INVALID_PARAMETER2;
    }

    DumpInt15MemoryMap( MemoryMap );

    endAddress = StartAddress + BytesToCut;

    LOG( "Will cut %d.%d MBytes, starting from address: 0x%X\n", BytesToCut / MB_SIZE, ( BytesToCut % MB_SIZE ) / KB_SIZE, StartAddress );

    for(    pCurEntry = MemoryMap->MemoryMap.Flink;
            pCurEntry != &MemoryMap->MemoryMap;
            pCurEntry = pCurEntry->Flink
            )
    {
        pMap = CONTAINING_RECORD( pCurEntry, INT15_MEMORY_MAP_LIST_ENTRY, ListEntry );

        if (MemoryMapTypeUsableRAM != pMap->MemoryMapEntry.Type)
        {
            // we need to reserve usable RAM
            continue;
        }

        // we have 3 cases:
        // I.   StartAddress == MemoryMapEntry.StartAddress
        // II.  EndAddress == MemoryMapEntry.EndAddress
        // III. Region we want to reserve is (MemoryMapEntry.StartAddress, MemoryMapEntry.EndAddress)

        if( ( pMap->MemoryMapEntry.BaseAddress <= StartAddress ) && ( pMap->MemoryMapEntry.BaseAddress + pMap->MemoryMapEntry.Length >= endAddress ) )
        {
            // case I.
            if( pMap->MemoryMapEntry.BaseAddress == StartAddress )
            {
                // we just reduce this entry and it's over :)
                // it's even ok if the lengths are equal too, because the OS will ignore any entries we
                // return with 0 value :)
                pMap->MemoryMapEntry.BaseAddress = pMap->MemoryMapEntry.BaseAddress + BytesToCut;
                pMap->MemoryMapEntry.Length = pMap->MemoryMapEntry.Length - BytesToCut;
                found = TRUE;
                break;
            }

            // case II.
            if( pMap->MemoryMapEntry.BaseAddress + pMap->MemoryMapEntry.Length == endAddress )
            {
                // we reduce map entry
                pMap->MemoryMapEntry.Length = pMap->MemoryMapEntry.Length - BytesToCut;

                // we will insert the reserved area before this entry
                pCurEntry = pCurEntry->Blink;
                found = TRUE;
                break;
            }

            // case III.

            // this is the 'worst' scenario
            // we have to create another entry
            pNewEntry = HvAllocPoolWithTag( PoolAllocateZeroMemory, sizeof( INT15_MEMORY_MAP_LIST_ENTRY ), HEAP_VMX_TAG, 0 );
            if( NULL == pNewEntry )
            {
                return STATUS_HEAP_INSUFFICIENT_RESOURCES;
            }

            // the new entry will start from the end of our reserved area
            pNewEntry->MemoryMapEntry.BaseAddress = endAddress;
            pNewEntry->MemoryMapEntry.Length = ( pMap->MemoryMapEntry.BaseAddress + pMap->MemoryMapEntry.Length ) - endAddress;
            pNewEntry->MemoryMapEntry.ExtendedAttributes = MEMORY_MAP_ENTRY_EA_VALID_ENTRY;
            pNewEntry->MemoryMapEntry.Type = pMap->MemoryMapEntry.Type;
            InsertHeadList( &(pMap->ListEntry), &(pNewEntry->ListEntry ) );
            MemoryMap->NumberOfEntries = MemoryMap->NumberOfEntries + 1;

            // reduce previous memory map entry by our size
            pMap->MemoryMapEntry.Length = StartAddress - pMap->MemoryMapEntry.BaseAddress;

            found = TRUE;
            break;
        }
    }

    if( found )
    {
        ASSERT(NULL != pCurEntry);

        // in either of the 3 cases we need to create a reserved entry
        pReservedEntry = HvAllocPoolWithTag(PoolAllocateZeroMemory, sizeof(INT15_MEMORY_MAP_LIST_ENTRY), HEAP_VMX_TAG, 0);
        if (NULL == pReservedEntry)
        {
            return STATUS_HEAP_INSUFFICIENT_RESOURCES;
        }

        pReservedEntry->MemoryMapEntry.BaseAddress = StartAddress;
        pReservedEntry->MemoryMapEntry.Length = BytesToCut;
        pReservedEntry->MemoryMapEntry.ExtendedAttributes = MEMORY_MAP_ENTRY_EA_VALID_ENTRY;
        pReservedEntry->MemoryMapEntry.Type = MemoryMapTypeReserved;

        InsertHeadList(pCurEntry, &pReservedEntry->ListEntry);

        DumpInt15MemoryMap( MemoryMap );

        return STATUS_SUCCESS;
    }

    return status;
}

STATUS
SimulateInt15h(
    IN      INT15_MEMORY_MAP*       MemoryMap
    )
{
    STATUS status;
    DWORD eax;      // a.k.a functionCode - P2
    DWORD ebx;      // a.k.a continuation - P3
    DWORD es;       // es:di forms buffer pointer - P4
    DWORD di;       // di
    DWORD ecx;      // buffer size - P5
    DWORD edx;      // a.k.a signature - P6
    REGISTER_AREA* pState;

    PVOID bufferAddress;
    INT15_MEMORY_MAP_LIST_ENTRY* pMapEntry;

    static DWORD lastContinuation = 0;
    static LIST_ENTRY* currentEntry = NULL;

    if( NULL == MemoryMap )
    {
        return STATUS_INVALID_PARAMETER1;
    }

    status = STATUS_SUCCESS;

    pState = &GetCurrentVcpu()->ProcessorState->RegisterArea;

    eax = ( DWORD ) pState->RegisterValues[RegisterRax];
    ebx = ( DWORD ) pState->RegisterValues[RegisterRbx];
    di = ( DWORD ) pState->RegisterValues[RegisterRdi];
    ecx = ( DWORD ) pState->RegisterValues[RegisterRcx];
    edx = ( DWORD ) pState->RegisterValues[RegisterRdx];

    bufferAddress = NULL;
    pMapEntry = NULL;

    if( INT15_E820 != eax )
    {
        LOGL( "Fail\n" );
        goto fail;
        //return STATUS_INVALID_PARAMETER1;
    }

    if( 0 == ebx )
    {
        lastContinuation = 0;
    }


    if( 0 == lastContinuation )
    {
        currentEntry = MemoryMap->MemoryMap.Flink;
    }

    if( lastContinuation != ebx )
    {
        LOGL( "Fail\n" );
        goto fail;
        //return STATUS_INVALID_PARAMETER2;
    }

    if( ecx < INT15_ENTRY_SIZE )
    {
        LOGL( "Fail\n" );
        goto fail;
        //return STATUS_INVALID_PARAMETER5;
    }

    if( INT15_SIGNATURE != edx )
    {
        LOGL( "Fail\n" );
        goto fail;
       // return STATUS_INVALID_PARAMETER6;
    }

    es = (DWORD) VmxRead(VMCS_GUEST_ES_BASE);

    pMapEntry = CONTAINING_RECORD( currentEntry, INT15_MEMORY_MAP_LIST_ENTRY, ListEntry );

    currentEntry = currentEntry->Flink;

    // seems no shifting is needed (because of the way VMX works :)

// warning C4312: 'type cast': conversion from 'DWORD' to 'PVOID' of greater size
#pragma warning(suppress:4312)
    bufferAddress = ( PVOID ) ( es + di );
    bufferAddress = MapMemory(  bufferAddress,
                                ecx
                                );
    if( NULL == bufferAddress )
    {
        return STATUS_MEMORY_CANNOT_BE_MAPPED;
    }

    memcpy( bufferAddress, pMapEntry, ecx );
    UnmapMemory( bufferAddress, ecx );

    if( currentEntry == &( MemoryMap->MemoryMap ) )
    {
        // we reached the end
        lastContinuation = 0;
    }
    else
    {
        lastContinuation = lastContinuation + 1;
    }

    pState->RegisterValues[RegisterRax] = INT15_SIGNATURE;
    pState->RegisterValues[RegisterRbx] = lastContinuation;

    // we clear carry for success
    pState->Rflags = pState->Rflags & ~( RFLAGS_CARRY_FLAG_BIT );

    return status;

fail:
    LOG( "We're at fail\n" );
    lastContinuation = 0;
    pState->Rflags = pState->Rflags | ( RFLAGS_CARRY_FLAG_BIT );

    pState->RegisterValues[RegisterRbx] = lastContinuation;

    return STATUS_SUCCESS;
}

STATUS
PatchInt15(
    void
    )
{
    STATUS status;
    WORD* address;
    WORD segmentSelector;
    WORD offset;
    BYTE* pOldIntHandler;
    BYTE* pNewIvtHandler;

    status = STATUS_SUCCESS;
    pNewIvtHandler = NULL;

    pOldIntHandler = MapMemory( NULL, PAGE_SIZE );
    if( NULL == pOldIntHandler )
    {
        return STATUS_MEMORY_CANNOT_BE_MAPPED;
    }

    pOldIntHandler = ( BYTE*) pOldIntHandler + IVT_15H_ENTRY * IVT_ENTRY_SIZE;
    address = (WORD*)pOldIntHandler;
    offset = *address;
    *address = IVT_NEW_OFFSET;
    *(address - 2 ) = IVT_NEW_OFFSET + OFFSET_TO_VMCALL;
    segmentSelector = *( address + 1 );
    *(address + 1 ) = IVT_NEW_SELECTOR;
    *(address - 1 ) = IVT_NEW_SELECTOR;

    LOG( "Original INT 15H handler at: 0x%x\n", ( segmentSelector << 4 ) + offset );

    *(WORD*)( &PATCH_CODE[OFFSET_TO_OFFSET] ) = offset;
    *(WORD*)( &PATCH_CODE[OFFSET_TO_SELECTOR] ) = segmentSelector;

    pNewIvtHandler = MapMemory( (PVOID) IVT_NEW_ADDRESS, NO_OF_BYTES_TO_PATCH);
    if (NULL == pNewIvtHandler)
    {
        return STATUS_MEMORY_CANNOT_BE_MAPPED;
    }

    memcpy( pNewIvtHandler, (BYTE*)PATCH_CODE, NO_OF_BYTES_TO_PATCH );

    LOG("Will unmap VA for old handler: 0x%X\n", pOldIntHandler);
    LOG("Will unmap VA for new handler: 0x%X\n", pNewIvtHandler);

    UnmapMemory( pOldIntHandler, PAGE_SIZE );
    UnmapMemory( pNewIvtHandler, PAGE_SIZE );

    return status;
}
#pragma warning(pop)
