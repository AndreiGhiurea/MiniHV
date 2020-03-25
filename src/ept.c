#include "ept.h"
#include "log.h"
#include "hv_heap.h"
#include "native/memory.h"
#include "data.h"

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
    STATUS status = STATUS_SUCCESS;
    PVOID pEptTable = NULL;

    if (NULL == Ept)
    {
        return STATUS_INVALID_PARAMETER1;
    }

    pEptTable = HvAllocPoolWithTag(PoolAllocateZeroMemory, PAGE_SIZE, HEAP_EPT_TAG, PAGE_SIZE);
    if (NULL == pEptTable)
    {
        LOGL("HvAllocPoolWithTag failed\n");
        return STATUS_HEAP_INSUFFICIENT_RESOURCES;
    }

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
    BYTE* startAddress = NULL;
    BYTE* endAddress = NULL;

    EPT_PML4_ENTRY* pml4Entries = NULL;
    EPT_PDPT_ENTRY_PD* pdptEntries = NULL;
    EPT_PD_ENTRY_PT* pdEntries = NULL;
    EPT_PT_ENTRY* ptEntries = NULL;

    WORD pageOffset;
    WORD pteOffset;
    WORD pdeOffset;
    WORD pdpteOffset;
    WORD pml4Offset;

    if (0 == Size)
    {
        return NULL;
    }

    startAddress = (PVOID)AlignAddressLower(GuestPA, PAGE_SIZE);
    endAddress = (BYTE*)GuestPA + Size;
    for (; startAddress < endAddress; startAddress += PAGE_SIZE)
    {
        pml4Offset = MASK_EPT_PML4_OFFSET(startAddress);
        pdpteOffset = MASK_EPT_PDPTE_OFFSET(startAddress);
        pdeOffset = MASK_EPT_PDE_OFFSET(startAddress);
        pteOffset = MASK_EPT_PTE_OFFSET(startAddress);
        pageOffset = MASK_EPT_PAGE_OFFSET(startAddress);

        pml4Entries = (EPT_PML4_ENTRY*)PA2VA(gGlobalData.VmxCurrentSettings.Ept.PhysicalAddress << SHIFT_FOR_EPT_PHYSICAL_ADDR);
        pml4Entries = &(pml4Entries[pml4Offset]);
        if (!IsEptEntryPresent(pml4Entries))
        {
            InitAndAllocEptStructure(pml4Entries, PAGE_SIZE, NULL, TRUE, MemoryType, MAX_BYTE, FALSE);
        }

        pdptEntries = (EPT_PDPT_ENTRY_PD*)PA2VA(pml4Entries->PhysicalAddress << SHIFT_FOR_EPT_PHYSICAL_ADDR);
        pdptEntries = &(pdptEntries[pdpteOffset]);
        if (!IsEptEntryPresent(pdptEntries))
        {
            InitAndAllocEptStructure(pdptEntries, PAGE_SIZE, NULL, TRUE, MemoryType, MAX_BYTE, FALSE);
        }

        pdEntries = (EPT_PD_ENTRY_PT*)PA2VA(pdptEntries->PhysicalAddress << SHIFT_FOR_EPT_PHYSICAL_ADDR);
        pdEntries = &(pdEntries[pdeOffset]);
        if (!IsEptEntryPresent(pdEntries))
        {
            InitAndAllocEptStructure(pdEntries, PAGE_SIZE, NULL, TRUE, MemoryType, MAX_BYTE, FALSE);
        }

        ptEntries = (EPT_PT_ENTRY*)PA2VA(pdEntries->PhysicalAddress << SHIFT_FOR_EPT_PHYSICAL_ADDR);
        ptEntries = &(ptEntries[pteOffset]);
        if ((!IsEptEntryPresent(ptEntries)) || (Overwrite))
        {
            InitAndAllocEptStructure(ptEntries, PAGE_SIZE, (PVOID)((QWORD)startAddress), FALSE, MemoryType, MAX_BYTE, Invalidate);
        }
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

    if (NULL == PageTable)
    {
        return;
    }

    if (0 == Size)
    {
        return;
    }

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