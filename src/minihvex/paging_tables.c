#include "paging_tables.h"
#include "log.h"
#include "hv_heap.h"
#include "native/memory.h"
#include "data.h"


//******************************************************************************
// Function:      InitAndAllocStructure
// Description: Initializes the paging structure(PML4,PDTPE,PDTE,PTE) and in
//              case the structure points to mapped memory also 'allocate' it.
// Returns:       void
// Parameter:     IN PVOID PageTable           - Structure to initialize
// Parameter:     IN DWORD Size                - Size of structure it maps
// Parameter:     IN_OPT PVOID PhysicalAddress - if non-NULL PA it maps
//******************************************************************************
static
void
InitAndAllocStructure(
    IN          PVOID       PageTable,
    IN          DWORD       Size,
    IN_OPT      PVOID       PhysicalAddress,
    IN          BOOLEAN     UsePhysicalMemory,
    IN_OPT      BYTE        MemoryType
    );

PTR_SUCCESS
PVOID
MapMemoryInvalidate(
    IN_OPT  PVOID           PhysicalAddress,
    IN      DWORD           Size,
    IN      BYTE            MemoryType,
    IN      BOOLEAN         Invalidate
    )
{
    PML4_ENTRY* pml4Entries;
    PDPT_ENTRY_PD* pdptEntries;
    PD_ENTRY_PT* pdEntries;
    PT_ENTRY* ptEntries;
    PHYSICAL_ADDRESS cr3;

    WORD pageOffset;
    WORD pteOffset;
    WORD pdeOffset;
    WORD pdpteOffset;
    WORD pml4Offset;

    DWORD offset;
    PVOID tempAddress;
    PVOID alignedAddress;
    BYTE* endAllocation;

    // we currently don't know how to map other memory types
    ASSERT(MEMORY_TYPE_WRITEBACK == MemoryType || MEMORY_TYPE_STRONG_UNCACHEABLE == MemoryType);

    cr3 = __readcr3();

    ASSERT( NULL != cr3 );

    if( 0 == Size )
    {
        return NULL;
    }

    if( 0 != gGlobalData.ApicData.ActiveCpus )
    {
        // WE NEED to take spinlock here
        AcquireLock( &( gGlobalData.PagingData.PagingLock ) );
    }

    offset = 0;

    // we need to page align the address
    alignedAddress = ( PVOID ) AlignAddressLower(PhysicalAddress,PAGE_SIZE);
    endAllocation =  ( BYTE* ) PhysicalAddress + Size;

    // we may need to map multiple pages => we iterate until we map all the
    // addresses
    while( ( ( BYTE* ) alignedAddress + offset ) < endAllocation )
    {
        // address to map
        tempAddress = ( PVOID ) PA2VA((BYTE*)alignedAddress+offset);

        // these are the offsets to the corresponding structures
        pml4Offset = MASK_PML4_OFFSET(tempAddress);
        pdpteOffset = MASK_PDPTE_OFFSET(tempAddress);
        pdeOffset = MASK_PDE_OFFSET(tempAddress);
        pteOffset = MASK_PTE_OFFSET(tempAddress);
        pageOffset = MASK_PAGE_OFFSET(tempAddress);

        pml4Entries = ( PML4_ENTRY* )  PA2VA( cr3 );
        pml4Entries = &(pml4Entries[pml4Offset]);

        if( 0 == pml4Entries->Present )
        {
            // We will allocate a PML4E to point to a PDPT
            InitAndAllocStructure( pml4Entries, PAGE_SIZE, NULL, FALSE, MemoryType );
        }

        tempAddress = ( PVOID ) ( pml4Entries->PhysicalAddress << SHIFT_FOR_PHYSICAL_ADDR );
        pdptEntries = ( PDPT_ENTRY_PD* )  PA2VA( tempAddress );

        pdptEntries = &(pdptEntries[pdpteOffset]);

        if( 0 == pdptEntries->Present )
        {
            // We will allocate a PDPT to point to a PD
            InitAndAllocStructure( pdptEntries, PAGE_SIZE, NULL, FALSE, MemoryType );
        }

        ASSERT(0 == pdptEntries->PageSize);

        tempAddress = ( PVOID ) ( pdptEntries->PhysicalAddress << SHIFT_FOR_PHYSICAL_ADDR );
        pdEntries = ( PD_ENTRY_PT* )  PA2VA( tempAddress );

        pdEntries = &(pdEntries[pdeOffset]);
        if( 0 == pdEntries->Present )
        {
            // We will allocate a PDE to point to a PT
            InitAndAllocStructure( pdEntries, PAGE_SIZE, NULL, FALSE, MemoryType );
        }

        ASSERT(0 == pdEntries->PageSize);

        tempAddress = ( PVOID ) ( pdEntries->PhysicalAddress << SHIFT_FOR_PHYSICAL_ADDR );
        ptEntries = ( PT_ENTRY* )  PA2VA( tempAddress );


        ptEntries = &(ptEntries[pteOffset] );

        // if we must invalidate the entry or the entry is not present, map it
        if( Invalidate || ( 0 == ptEntries->Present ) )
        {
            // we will map a PTE to a physical page
            InitAndAllocStructure( ptEntries, PAGE_SIZE, ( PVOID ) ( ( QWORD ) alignedAddress + offset ), TRUE, MemoryType );
        }

        offset = offset + PAGE_SIZE;
    }

    if( 0 != gGlobalData.ApicData.ActiveCpus )
    {
        // WE NEED to take spinlock here
        ReleaseLock( &( gGlobalData.PagingData.PagingLock ) );
    }


    return ( PVOID ) PA2VA(PhysicalAddress);
}

STATUS
UnmapMemory(
    IN      PVOID       LogicalAddress,
    IN      DWORD       Size
    )
{
    PML4_ENTRY* pml4Entries;
    PDPT_ENTRY_PD* pdptEntries;
    PD_ENTRY_PT* pdEntries;
    PT_ENTRY* ptEntries;
    PHYSICAL_ADDRESS cr3;

    WORD pageOffset;
    WORD pteOffset;
    WORD pdeOffset;
    WORD pdpteOffset;
    WORD pml4Offset;

    QWORD offset;
    PVOID tempAddress;
    PVOID alignedAddress;
    BYTE* unmappingEnd;

    cr3 = __readcr3();
    ASSERT(NULL != cr3);

    if( NULL == LogicalAddress )
    {
        return STATUS_INVALID_PARAMETER1;
    }

    if( 0 == Size )
    {
        return STATUS_INVALID_PARAMETER2;
    }

    // if we're the only ones here we don't need any locks :)
    if( 0 != gGlobalData.ApicData.ActiveCpus )
    {
        // WE NEED to take spinlock here
        AcquireLock( &( gGlobalData.PagingData.PagingLock ) );
    }

    offset = 0;

    // we need to page align the address
    alignedAddress = ( PVOID ) AlignAddressLower(LogicalAddress,PAGE_SIZE);
    unmappingEnd =  ( BYTE* ) LogicalAddress + Size;

    // we may need to map multiple pages => we iterate until we map all the
    // addresses
    while( ( ( BYTE* ) alignedAddress + offset ) < unmappingEnd )
    {
        // address to ummap
        tempAddress = ( PVOID ) ((BYTE*)alignedAddress+offset);

        // these are the offsets to the corresponding structures
        pml4Offset = MASK_PML4_OFFSET(tempAddress);
        pdpteOffset = MASK_PDPTE_OFFSET(tempAddress);
        pdeOffset = MASK_PDE_OFFSET(tempAddress);
        pteOffset = MASK_PTE_OFFSET(tempAddress);
        pageOffset = MASK_PAGE_OFFSET(tempAddress);

        pml4Entries = ( PML4_ENTRY* )  PA2VA( cr3 );
        pml4Entries = &(pml4Entries[pml4Offset]);

        if( 0 == pml4Entries->Present )
        {
            // it was never allocated
            // go to next PAGE
            goto endloop;
        }

        tempAddress = ( PVOID ) ( pml4Entries->PhysicalAddress << SHIFT_FOR_PHYSICAL_ADDR );
        pdptEntries = ( PDPT_ENTRY_PD* )  PA2VA( tempAddress );

        pdptEntries = &(pdptEntries[pdpteOffset]);

        if( 0 == pdptEntries->Present )
        {
            // it was never allocated
            // go to next PAGE
            goto endloop;
        }

        ASSERT(0 == pdptEntries->PageSize);

        tempAddress = ( PVOID ) ( pdptEntries->PhysicalAddress << SHIFT_FOR_PHYSICAL_ADDR );
        pdEntries = ( PD_ENTRY_PT* )  PA2VA( tempAddress );

        pdEntries = &(pdEntries[pdeOffset]);
        if( 0 == pdEntries->Present )
        {
            // it was never allocated
            // go to next PAGE
            goto endloop;
        }

        ASSERT(0 == pdEntries->PageSize);

        tempAddress = ( PVOID ) ( pdEntries->PhysicalAddress << SHIFT_FOR_PHYSICAL_ADDR );
        ptEntries = ( PT_ENTRY* )  PA2VA( tempAddress );


        ptEntries = &(ptEntries[pteOffset] );

        if( 0 == ptEntries->Present )
        {
            // it was never allocated
            // go to next PAGE
            goto endloop;
        }
        else
        {
            memzero( ptEntries, sizeof( PT_ENTRY ) );

            PageInvalidateTlb( (PBYTE) alignedAddress + offset );
        }

endloop:
        offset = offset + PAGE_SIZE;
    }

    // if we're the only ones here we don't need any locks :)
    if( 0 != gGlobalData.ApicData.ActiveCpus )
    {
        // WE NEED to take spinlock here
        ReleaseLock( &( gGlobalData.PagingData.PagingLock ) );
    }

    return STATUS_SUCCESS;
}

void
UnmapPML4Entry(
    IN_OPT  PVOID       LogicalAddress
    )
{
    PML4_ENTRY* pml4Table;
    PML4_ENTRY* pml4Entries;
    WORD pml4Offset;
    PHYSICAL_ADDRESS cr3;

    cr3 = __readcr3();
    ASSERT(NULL != cr3);

    pml4Table = NULL;
    pml4Entries = NULL;
    pml4Offset = 0;

    // if we're the only ones here we don't need any locks :)
    if (0 != gGlobalData.ApicData.ActiveCpus)
    {
        // WE NEED to take spinlock here
        AcquireLock(&(gGlobalData.PagingData.PagingLock));
    }

    // these are the offsets to the corresponding structures
    pml4Offset = MASK_PML4_OFFSET(LogicalAddress);

    pml4Table = (PML4_ENTRY*)PA2VA(cr3);
    pml4Entries = &(pml4Table[pml4Offset]);

    if (1 == pml4Entries->Present)
    {
        // unmap PML4 entry
        memzero(pml4Entries, sizeof(PML4_ENTRY));

        PageInvalidateTlb(pml4Entries);
    }

    // if we're the only ones here we don't need any locks :)
    if (0 != gGlobalData.ApicData.ActiveCpus)
    {
        // WE NEED to take spinlock here
        ReleaseLock(&(gGlobalData.PagingData.PagingLock));
    }
}

PVOID
VA64toPA(
    IN      PML4*       Pml4,
    IN      PVOID       LogicalAddress
    )
{
    PML4_ENTRY* pml4Entries;
    PDPT_ENTRY_PD* pdptEntries;
    PDPT_ENTRY_1G* pdpt1Gb;
    PD_ENTRY_2MB* pd2Mb;
    PD_ENTRY_PT* pdEntries;
    PT_ENTRY* ptEntries;

    WORD pageOffset;
    WORD pteOffset;
    WORD pdeOffset;
    WORD pdpteOffset;
    WORD pml4Offset;

    PVOID tempAddress;

    if( 0 == Pml4 )
    {
        return NULL;
    }

    if( NULL == LogicalAddress )
    {
        return NULL;
    }


    // these are the offsets to the corresponding structures
    pml4Offset = MASK_PML4_OFFSET(LogicalAddress);
    pdpteOffset = MASK_PDPTE_OFFSET(LogicalAddress);
    pdeOffset = MASK_PDE_OFFSET(LogicalAddress);
    pteOffset = MASK_PTE_OFFSET(LogicalAddress);
    pageOffset = MASK_PAGE_OFFSET(LogicalAddress);

    pml4Entries = ( PML4_ENTRY* )  MapMemory( (PVOID)GPA2HPA( Pml4->NoPcide.PhysicalAddress << SHIFT_FOR_PHYSICAL_ADDR ), PAGE_SIZE );
    pml4Entries = &(pml4Entries[pml4Offset]);

    if( 0 == pml4Entries->Present )
    {
        LOGL( "We are here\n" );
        return NULL;
    }

    tempAddress = ( PVOID ) ( pml4Entries->PhysicalAddress << SHIFT_FOR_PHYSICAL_ADDR );
    pdptEntries = ( PDPT_ENTRY_PD* )  MapMemory( (PVOID)GPA2HPA( tempAddress ), PAGE_SIZE );

    pdptEntries = &(pdptEntries[pdpteOffset]);

    if( 0 == pdptEntries->Present )
    {
        LOGL( "We are here\n" );
        return NULL;
    }

    if( 1 == pdptEntries->PageSize )
    {
        // we have a 1Gig page
        pdpt1Gb = ( PDPT_ENTRY_1G* ) pdptEntries;
        tempAddress = ( PVOID )( pdpt1Gb->PhysicalAddress << 30 );
        return ( BYTE* ) tempAddress + AddressOffset(LogicalAddress,1*GB_SIZE);
    }

    tempAddress = ( PVOID ) ( pdptEntries->PhysicalAddress << SHIFT_FOR_PHYSICAL_ADDR );
    pdEntries = ( PD_ENTRY_PT* )  MapMemory( (PVOID)GPA2HPA( tempAddress ), PAGE_SIZE );

    pdEntries = &(pdEntries[pdeOffset]);
    if( 0 == pdEntries->Present )
    {
        LOGL( "We are here\n" );
        return NULL;
    }

    if( 1 == pdEntries->PageSize )
    {
        // this is the end of the line
        pd2Mb = ( PD_ENTRY_2MB* ) pdEntries;
        tempAddress = ( PVOID )( pd2Mb->PhysicalAddress << 21 );
        return ( BYTE* ) tempAddress + AddressOffset(LogicalAddress,2*MB_SIZE);
    }

    tempAddress = ( PVOID ) ( pdEntries->PhysicalAddress << SHIFT_FOR_PHYSICAL_ADDR );
    ptEntries = ( PT_ENTRY* )  MapMemory( (PVOID)GPA2HPA( tempAddress ), PAGE_SIZE );

    ptEntries = &(ptEntries[pteOffset] );

    if( 0 == ptEntries->Present )
    {
        LOGL( "We are here\n" );
        return NULL;
    }

    tempAddress = ( PVOID ) ( ptEntries->PhysicalAddress << SHIFT_FOR_PHYSICAL_ADDR );

    return ( BYTE* ) tempAddress + AddressOffset(LogicalAddress,4*KB_SIZE);
}

PVOID
VAPAEtoPA(
    IN        CR3_PAE_STRUCTURE*    Cr3,
    IN        PVOID                LogicalAddress
)
{
    PDPT_PAE_ENTRY_PD* tableOfPdpts;
    PDPT_PAE_ENTRY_PD* pointerToPageDirectory;
    PD_PAE_ENTRY_PT* pageDirectory;
    PD_PAE_ENTRY_PT* pageDirectoryEntry;
    PD_PAE_ENTRY_2MB* pBigPage;
    PT_PAE_ENTRY* pageTable;
    PT_PAE_ENTRY* pageTableEntry;

    WORD pteOffset;
    WORD pdeOffset;
    WORD pdpteOffset;

    PVOID tempAddress;

    if (0 == Cr3)
    {
        return NULL;
    }

    if (NULL == LogicalAddress)
    {
        return NULL;
    }


    // these are the offsets to the corresponding structures
    pdpteOffset = MASK_PAE_PDPTE_OFFSET(LogicalAddress);
    pdeOffset = MASK_PDE_OFFSET(LogicalAddress);
    pteOffset = MASK_PTE_OFFSET(LogicalAddress);

    tableOfPdpts = (PDPT_PAE_ENTRY_PD*)MapMemory((PVOID)GPA2HPA(Cr3->PhysicalAddress << 5), PAGE_SIZE);
    pointerToPageDirectory = &(tableOfPdpts[pdpteOffset]);

    if (0 == pointerToPageDirectory->Present)
    {
        LOGL("We are here, PDPTE value: 0x%X\n", *((QWORD*)pointerToPageDirectory));
        return NULL;
    }

    pageDirectory = (PD_PAE_ENTRY_PT*)MapMemory((PVOID)GPA2HPA(pointerToPageDirectory->PhysicalAddress << SHIFT_FOR_PHYSICAL_ADDR), PAGE_SIZE);
    pageDirectoryEntry = &(pageDirectory[pdeOffset]);

    if (0 == pageDirectoryEntry->Present)
    {
        LOGL("We are here, PD value: 0x%X\n", *((QWORD*)pageDirectoryEntry));
        return NULL;
    }

    if (1 == pageDirectoryEntry->PageSize)
    {
        // 2 MB
        pBigPage = (PD_PAE_ENTRY_2MB*)pageDirectoryEntry;
        tempAddress = (PVOID)(pBigPage->PhysicalAddress << 21);
        return (BYTE*)tempAddress + AddressOffset(LogicalAddress, 2 * MB_SIZE);
    }

    tempAddress = (PVOID)(pageDirectoryEntry->PhysicalAddress << SHIFT_FOR_PHYSICAL_ADDR);
    pageTable = (PT_PAE_ENTRY*)MapMemory((PVOID)GPA2HPA(tempAddress), PAGE_SIZE);

    pageTableEntry = &(pageTable[pteOffset]);

    if (0 == pageTableEntry->Present)
    {
        LOGL("We are here\n");
        return NULL;
    }

    tempAddress = (PVOID)(pageTableEntry->PhysicalAddress << SHIFT_FOR_PHYSICAL_ADDR);

    return (BYTE*)tempAddress + AddressOffset(LogicalAddress, 4 * KB_SIZE);
}


PVOID
VA32toPA(
    IN        CR3_STRUCTURE*        Cr3,
    IN        PVOID                LogicalAddress
)
{
    PD32_ENTRY_PT* pageDirectory;
    PD32_ENTRY_PT* pageDirectoryEntry;
    PD32_ENTRY_4MB* pBigPage;
    PT32_ENTRY* pageTable;
    PT32_ENTRY* pageTableEntry;

    WORD pteOffset;
    WORD pdeOffset;

    PVOID tempAddress;

    if (0 == Cr3)
    {
        return NULL;
    }

    if (NULL == LogicalAddress)
    {
        return NULL;
    }


    // these are the offsets to the corresponding structures
    pdeOffset = MASK_PDE32_OFFSET(LogicalAddress);
    pteOffset = MASK_PTE32_OFFSET(LogicalAddress);

    pageDirectory = (PD32_ENTRY_PT*)MapMemory((PVOID)GPA2HPA(Cr3->PhysicalAddress << SHIFT_FOR_PHYSICAL_ADDR), PAGE_SIZE);
    pageDirectoryEntry = &(pageDirectory[pdeOffset]);

    if (0 == pageDirectoryEntry->Present)
    {
        LOGL("We are here\n");
        return NULL;
    }

    if (1 == pageDirectoryEntry->PageSize)
    {
        // 4 MB
        pBigPage = (PD32_ENTRY_4MB*)pageDirectoryEntry;
        tempAddress = (PVOID)((((QWORD)pBigPage->UpperBits << 10) |(QWORD) pBigPage->PhysicalAddress) << 22);
        return (BYTE*)tempAddress + AddressOffset(LogicalAddress, 4 * MB_SIZE);
    }

    tempAddress = (PVOID)((QWORD)pageDirectoryEntry->PhysicalAddress << SHIFT_FOR_PHYSICAL_ADDR);
    pageTable = (PT32_ENTRY*)MapMemory((PVOID)GPA2HPA(tempAddress), PAGE_SIZE);

    pageTableEntry = &(pageTable[pteOffset]);

    if (0 == pageTableEntry->Present)
    {
        LOGL("We are here\n");
        return NULL;
    }

    tempAddress = (PVOID)((QWORD)pageTableEntry->PhysicalAddress << SHIFT_FOR_PHYSICAL_ADDR);

    return (BYTE*)tempAddress + AddressOffset(LogicalAddress, 4 * KB_SIZE);
}

static
void
InitAndAllocStructure(
    IN          PVOID       PageTable,
    IN          DWORD       Size,
    IN_OPT      PVOID       PhysicalAddress,
    IN          BOOLEAN     UsePhysicalMemory,
    IN_OPT      BYTE        MemoryType
    )
{
    PT_ENTRY* pTablePointer;
    PVOID tempAddress;
    QWORD convertedAddress;

    ASSERT( NULL != PageTable );
    ASSERT( 0 != Size );

    pTablePointer = PageTable;
    memzero( pTablePointer, sizeof( PVOID ) );

    // This entry is not present, map it accordingly
    if (UsePhysicalMemory)
    {
        BYTE memoryTypeIndex;

        // this entry will point to a mapping
        tempAddress = (PVOID)PA2VA(PhysicalAddress);
        convertedAddress = (QWORD)PhysicalAddress;

        // if we use physical memory we will also need to set the caching bits
        memoryTypeIndex = MEMORY_TYPE_STRONG_UNCACHEABLE == MemoryType ? gGlobalData.PagingData.UncacheableMemoryIndex : gGlobalData.PagingData.WriteBackMemoryIndex;

        pTablePointer->PAT = (memoryTypeIndex >> 2) & 1;
        pTablePointer->PCD = (memoryTypeIndex >> 1) & 1;
        pTablePointer->PWT = (memoryTypeIndex >> 0) & 1;
    }
    else
    {
        ASSERT(NULL == PhysicalAddress);

        // this entry will point to another pagination structure
        tempAddress = HvAllocPoolWithTag( PoolAllocatePanicIfFail | PoolAllocateZeroMemory, Size, HEAP_PAGE_TAG, PAGE_SIZE );
        convertedAddress = VA2PA( tempAddress );
    }

    pTablePointer->PhysicalAddress = convertedAddress >> SHIFT_FOR_PHYSICAL_ADDR;
    pTablePointer->Present = 1;
    pTablePointer->ReadWrite = 1;

    // we don't want our data to be executed :)
    pTablePointer->XD = 1;

    // 0 means user-mode accesses are forbidden
    pTablePointer->UserSupervisor = 0;

    PageInvalidateTlb( tempAddress );
}