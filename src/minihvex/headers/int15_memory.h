#ifndef _INT15_MEMORY_H_
#define _INT15_MEMORY_H_

#include "minihv.h"

#pragma pack(push,1)


typedef enum _MEMORY_MAP_TYPE
{
    MemoryMapTypeUsableRAM      = 1,
    MemoryMapTypeReserved,
    MemoryMapTypeACPIReclaimable,
    MemoryMapTypeACPINVSMemory,
    MemoryMapTypeBadMemory
} MEMORY_MAP_TYPE;

#define MEMORY_MAP_ENTRY_EA_VALID_ENTRY             ((DWORD)1<<0)
#define MEMORY_MAP_ENTRY_EA_NON_VOLATILE            ((DWORD)1<<1)

typedef struct _INT15_MEMORY_MAP_ENTRY
{
    QWORD           BaseAddress;
    QWORD           Length;
    DWORD           Type;
    DWORD           ExtendedAttributes;
} INT15_MEMORY_MAP_ENTRY, *PINT15_MEMORY_MAP_ENTRY;


typedef struct _INT15_MEMORY_MAP_LIST_ENTRY
{
    INT15_MEMORY_MAP_ENTRY          MemoryMapEntry;
    LIST_ENTRY                      ListEntry;
} INT15_MEMORY_MAP_LIST_ENTRY, *PINT15_MEMORY_MAP_LIST_ENTRY;
#pragma pack(pop)

typedef struct _INT15_MEMORY_MAP
{
    QWORD           NumberOfEntries;
    LIST_ENTRY      MemoryMap;
} INT15_MEMORY_MAP, *PINT15_MEMORY_MAP;

STATUS
Int15DetermineMemoryMapParameters(
    IN_READS(NoOfEntries)       INT15_MEMORY_MAP_ENTRY*     MemoryMap,
    IN                          DWORD                       NoOfEntries,
    OUT                         QWORD*                      AvailableSystemMemory,
    OUT                         QWORD*                      HighestAvailablePhysicalAddress
);

STATUS
Int15NormalizeMemoryMap(
    IN_READS(NoOfEntries)       INT15_MEMORY_MAP_ENTRY*     MemoryMap,
    IN                          DWORD                       NoOfEntries,
    OUT                         INT15_MEMORY_MAP*           NormalizedMemoryMap
);

#endif // _INT15_MEMORY_H_