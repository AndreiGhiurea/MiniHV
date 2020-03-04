#include "int15_memory.h"
#include "hv_heap.h"
#include "native/memory.h"

STATUS
Int15DetermineMemoryMapParameters(
    IN_READS(NoOfEntries)       INT15_MEMORY_MAP_ENTRY*     MemoryMap,
    IN                          DWORD                       NoOfEntries,
    OUT                         QWORD*                      AvailableSystemMemory,
    OUT                         QWORD*                      HighestAvailablePhysicalAddress
)
{
    DWORD i;
    QWORD sizeOfAvailableMemory;
    QWORD highestMemoryAddressAvailable;

    if( NULL == MemoryMap )
    {
        return STATUS_INVALID_PARAMETER1;
    }

    if( 0 == NoOfEntries )
    {
        return STATUS_INVALID_PARAMETER2;
    }

    if( NULL == AvailableSystemMemory )
    {
        return STATUS_INVALID_PARAMETER3;
    }

    if( NULL == HighestAvailablePhysicalAddress )
    {
        return STATUS_INVALID_PARAMETER4;
    }

    sizeOfAvailableMemory = 0;
    highestMemoryAddressAvailable = 0;

    for( i = 0; i < NoOfEntries; ++i )
    {
        if( MemoryMap[i].BaseAddress + MemoryMap[i].Length > highestMemoryAddressAvailable )
        {
            highestMemoryAddressAvailable = MemoryMap[i].BaseAddress + MemoryMap[i].Length;
        }

        if( !IsBooleanFlagOn( MemoryMap[i].ExtendedAttributes, MEMORY_MAP_ENTRY_EA_VALID_ENTRY ) )
        {
            // if this flag is clear => entry should be ignored
            continue;
        }

        if( MemoryMapTypeUsableRAM != MemoryMap[i].Type )
        {
            // we only care about usable RAM memory
            continue;
        }

        sizeOfAvailableMemory = sizeOfAvailableMemory + MemoryMap[i].Length;
    }

    *AvailableSystemMemory = sizeOfAvailableMemory;
    *HighestAvailablePhysicalAddress = highestMemoryAddressAvailable;

    return STATUS_SUCCESS;
}

STATUS
Int15NormalizeMemoryMap(
    IN_READS(NoOfEntries)       INT15_MEMORY_MAP_ENTRY*     MemoryMap,
    IN                          DWORD                       NoOfEntries,
    OUT                         INT15_MEMORY_MAP*           NormalizedMemoryMap
)
{
    DWORD i;
    DWORD noOfValidMemoryMapEntries;
    INT15_MEMORY_MAP_LIST_ENTRY* pNewEntry;

    if( NULL == MemoryMap )
    {
        return STATUS_INVALID_PARAMETER1;
    }

    if( 0 == NoOfEntries )
    {
        return STATUS_INVALID_PARAMETER2;
    }

    if( NULL == NormalizedMemoryMap )
    {
        return STATUS_INVALID_PARAMETER3;
    }

    i = 0;
    noOfValidMemoryMapEntries = 0;

    InitializeListHead( &( NormalizedMemoryMap->MemoryMap ) );
    

    for( i = 0; i < NoOfEntries; ++i )
    {
        if( !IsBooleanFlagOn( MemoryMap[i].ExtendedAttributes, MEMORY_MAP_ENTRY_EA_VALID_ENTRY ) )
        {
            // if this flag is clear => entry should be ignored
            continue;
        }

        // we shouldn't remove any entry from the memory map because the guest OS
        // could then map devices in that memory region

        pNewEntry = HvAllocPoolWithTag( PoolAllocateZeroMemory, sizeof( INT15_MEMORY_MAP_LIST_ENTRY ), HEAP_VMX_TAG, 0 );
        if( NULL == pNewEntry )
        {
            return STATUS_HEAP_INSUFFICIENT_RESOURCES;
        }

        memcpy( &(pNewEntry->MemoryMapEntry), (PVOID) (&MemoryMap[i]), sizeof(INT15_MEMORY_MAP_ENTRY));

        InsertTailList( &( NormalizedMemoryMap->MemoryMap ), &( pNewEntry->ListEntry ) );
        noOfValidMemoryMapEntries++;
    }

    NormalizedMemoryMap->NumberOfEntries = noOfValidMemoryMapEntries;

    return STATUS_SUCCESS;
}