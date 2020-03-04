#ifndef _PAGING_TABLES_H_
#define _PAGING_TABLES_H_

#include "minihv.h"
#include "lock.h"
#include "pte.h"


#define     EXPECTED_IA32_PAT_VALUES            0x0007'0406'0007'0406ULL

typedef struct _PAGING_DATA
{
    LOCK                        PagingLock;

    QWORD                       Ia32PatValues;

    BYTE                        UncacheableMemoryIndex;
    BYTE                        WriteBackMemoryIndex;
} PAGING_DATA, *PPAGING_DATA;

#define MapMemory(PA,Size)       MapMemoryInvalidate((PA),(Size),MEMORY_TYPE_WRITEBACK,FALSE)

//******************************************************************************
// Function:      MemoryMap
// Description: Maps a physical address to virtual memory.
// Returns:       PVOID - VA of the address mapped.
// Parameter:     IN PVOID PhysicalAddress - PA to map
// Parameter:     IN DWORD Size            - Number of bytes to map
//******************************************************************************
PTR_SUCCESS
PVOID
MapMemoryInvalidate(
    IN_OPT  PVOID       PhysicalAddress,
    IN      DWORD       Size,
    IN      BYTE        MemoryType,
    IN      BOOLEAN     Invalidate
    );

//******************************************************************************
// Function:      UnmapMemory
// Description: Unmaps a previously mapped PA.
// Returns:       STATUS
// Parameter:     IN PVOID LogicalAddress
// Parameter:     IN DWORD Size
//******************************************************************************
STATUS
UnmapMemory(
    IN      PVOID       LogicalAddress,
    IN      DWORD       Size
    );

void
UnmapPML4Entry(
    IN_OPT  PVOID       LogicalAddress
    );

//******************************************************************************
// Function:      VA64toPA
// Description: Converts a PA using an IA-32e paging structure to a VA.
// Returns:       PVOID
// Parameter:     IN PML4* Pml4
// Parameter:     IN PVOID LogicalAddress
//******************************************************************************
PVOID
VA64toPA(
    IN      PML4*       Pml4,
    IN      PVOID       LogicalAddress
    );

//******************************************************************************
// Function:    VAPAEtoPA
// Description: Converts a PA using a PAE paging structure to a VA.
// Returns:       PVOID
// Parameter:     IN CR3_PAE_STRUCTURE* Cr3
// Parameter:     IN PVOID LogicalAddress
//******************************************************************************
PVOID
VAPAEtoPA(
    IN        CR3_PAE_STRUCTURE*   Cr3,
    IN        PVOID                LogicalAddress
);

//******************************************************************************
// Function:    VA32toPA
// Description: Converts a PA using a 32-bit paging structure to a VA.
// Returns:       PVOID
// Parameter:     IN CR3_STRUCTURE* Cr3
// Parameter:     IN PVOID LogicalAddress
//******************************************************************************
PVOID
VA32toPA(
    IN        CR3_STRUCTURE*        Cr3,
    IN        PVOID                LogicalAddress
);

#endif // _PAGING_TABLES_H_