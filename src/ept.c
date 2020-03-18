#include "ept.h"
#include "log.h"
#include "hv_heap.h"
#include "native/memory.h"
#include "data.h"


//******************************************************************************
// Function:      InitAndAllocEptStructure
// Description: Creates an EPT paging structure or a mapping.
// Returns:       void
// Parameter:     IN PVOID PageTable - Structure which will describe new structure.
// Parameter:     IN DWORD Size - Size of the newly allocated structure.
// Parameter:     IN_OPT PVOID PhysicalAddress - Address of the mapping(used only
//              when CreateStructure is FALSE)
// Parameter:     IN BOOLEAN CreateStructure - if TRUE the function creates a
//              mapping to a new EPT structure, else it maps PhysicalAddress
// Parameter:   IN_OPT BYTE MemoryType - Used only if CreateStructure is FALSE
//******************************************************************************
static
void
InitAndAllocEptStructure(
    IN          PVOID       PageTable,
    IN          DWORD       Size,
    IN_OPT      PVOID       PhysicalAddress,
    IN          BOOLEAN     CreateStructure,
    IN_OPT      BYTE        MemoryType,
    IN          BYTE        RwxAccess,
    IN          BOOLEAN     Invalidate
);

STATUS
ConfigureEPTP(
    OUT     EPTP* Ept
)
{
    STATUS status;
    PVOID pEptTable;

    if (NULL == Ept)
    {
        return STATUS_INVALID_PARAMETER1;
    }

    status = STATUS_SUCCESS;
    pEptTable = NULL;

    pEptTable = HvAllocPoolWithTag(PoolAllocateZeroMemory, PAGE_SIZE, HEAP_EPT_TAG, PAGE_SIZE);
    if (NULL == pEptTable)
    {
        LOGL("HvAllocPoolWithTag failed\n");
        return STATUS_HEAP_INSUFFICIENT_RESOURCES;
    }

    ASSERT(gGlobalData.VmxConfigurationData.EptSupport.PageWalkLength4);
    Ept->PageWalkLength = 3;

    Ept->MemoryType = gGlobalData.VmxConfigurationData.EptSupport.MemoryType;
    Ept->PhysicalAddress = (QWORD)(VA2PA(pEptTable) >> SHIFT_FOR_EPT_PHYSICAL_ADDR);

    return status;
}

PVOID
EptMapGuestPA(
    IN      PVOID       GuestPA,
    IN      DWORD       Size,
    IN      BYTE        MemoryType,
    IN_OPT  PVOID       HostPA,
    IN      BYTE        RwxAccess,
    IN      BOOLEAN     Overwrite,
    IN      BOOLEAN     Invalidate
)
{
    PVOID result;

    EPT_PML4_ENTRY* pml4Entries;
    EPT_PDPT_ENTRY_PD* pdptEntries;
    EPT_PD_ENTRY_PT* pdEntries;
    EPT_PT_ENTRY* ptEntries;

    WORD pageOffset;
    WORD pteOffset;
    WORD pdeOffset;
    WORD pdpteOffset;
    WORD pml4Offset;

    DWORD offset;
    PVOID tempAddress;
    PVOID alignedAddress;
    BYTE* endAllocation;

    ASSERT(NULL != (PVOID)gGlobalData.VmxCurrentSettings.Ept.PhysicalAddress);

    if (0 == Size)
    {
        return NULL;
    }

    result = NULL;

    offset = 0;

    alignedAddress = (PVOID)AlignAddressLower(GuestPA, PAGE_SIZE);
    endAllocation = (BYTE*)GuestPA + Size;

    while (((BYTE*)alignedAddress + offset) < endAllocation)
    {
        tempAddress = (PVOID)((BYTE*)alignedAddress + offset);

        pml4Offset = MASK_EPT_PML4_OFFSET(tempAddress);
        pdpteOffset = MASK_EPT_PDPTE_OFFSET(tempAddress);
        pdeOffset = MASK_EPT_PDE_OFFSET(tempAddress);
        pteOffset = MASK_EPT_PTE_OFFSET(tempAddress);
        pageOffset = MASK_EPT_PAGE_OFFSET(tempAddress);

        pml4Entries = (EPT_PML4_ENTRY*)PA2VA(gGlobalData.VmxCurrentSettings.Ept.PhysicalAddress << SHIFT_FOR_EPT_PHYSICAL_ADDR);
        pml4Entries = &(pml4Entries[pml4Offset]);

        if (!IsEptEntryPresent(pml4Entries))
        {
            InitAndAllocEptStructure(pml4Entries, PAGE_SIZE, NULL, TRUE, MemoryType, MAX_BYTE, FALSE);
        }

        tempAddress = (PVOID)(pml4Entries->PhysicalAddress << SHIFT_FOR_EPT_PHYSICAL_ADDR);
        pdptEntries = (EPT_PDPT_ENTRY_PD*)PA2VA(tempAddress);

        pdptEntries = &(pdptEntries[pdpteOffset]);

        if (!IsEptEntryPresent(pdptEntries))
        {
            InitAndAllocEptStructure(pdptEntries, PAGE_SIZE, NULL, TRUE, MemoryType, MAX_BYTE, FALSE);
        }

        tempAddress = (PVOID)(pdptEntries->PhysicalAddress << SHIFT_FOR_EPT_PHYSICAL_ADDR);
        pdEntries = (EPT_PD_ENTRY_PT*)PA2VA(tempAddress);

        pdEntries = &(pdEntries[pdeOffset]);
        if (!IsEptEntryPresent(pdEntries))
        {
            InitAndAllocEptStructure(pdEntries, PAGE_SIZE, NULL, TRUE, MemoryType, MAX_BYTE, FALSE);
        }

        tempAddress = (PVOID)(pdEntries->PhysicalAddress << SHIFT_FOR_EPT_PHYSICAL_ADDR);
        ptEntries = (EPT_PT_ENTRY*)PA2VA(tempAddress);

        ptEntries = &(ptEntries[pteOffset]);

        if ((!IsEptEntryPresent(ptEntries)) || (Overwrite))
        {
            InitAndAllocEptStructure(ptEntries, PAGE_SIZE, (PVOID)((QWORD)alignedAddress + offset), FALSE, MemoryType, RwxAccess, Invalidate);
        }

        offset = offset + PAGE_SIZE;
    }

    return (PVOID)GPA2HPA(GuestPA);
}


static
void
InitAndAllocEptStructure(
    IN          PVOID       PageTable,
    IN          DWORD       Size,
    IN_OPT      PVOID       PhysicalAddress,
    IN          BOOLEAN     CreateStructure,
    IN_OPT      BYTE        MemoryType,
    IN          BYTE        RwxAccess,
    IN          BOOLEAN     Invalidate
)
{
    EPT_PT_ENTRY* pTablePointer;
    PVOID tempAddress;
    QWORD convertedAddress;

    ASSERT(NULL != PageTable);
    ASSERT(0 != Size);

    pTablePointer = PageTable;
    memzero(pTablePointer, sizeof(PVOID));

    if (CreateStructure)
    {
        tempAddress = HvAllocPoolWithTag(PoolAllocatePanicIfFail | PoolAllocateZeroMemory, Size, HEAP_EPT_TAG, PAGE_SIZE);
        convertedAddress = (QWORD)VA2PA(tempAddress);
    }
    else
    {
        convertedAddress = (QWORD)PhysicalAddress;

        pTablePointer->IgnorePAT = 1;
        pTablePointer->MemoryType = MemoryType;
    }


    pTablePointer->PhysicalAddress = convertedAddress >> SHIFT_FOR_PHYSICAL_ADDR;
    pTablePointer->Read = IsBooleanFlagOn(RwxAccess, EPT_READ_ACCESS);
    pTablePointer->Write = IsBooleanFlagOn(RwxAccess, EPT_WRITE_ACCESS);
    pTablePointer->Execute = IsBooleanFlagOn(RwxAccess, EPT_EXEC_ACCESS);
}