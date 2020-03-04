#include "mtrr.h"
#include "native/memory.h"
#include "hv_heap.h"
#include "data.h"

typedef struct _RANGE_SEARCH_STRUCT
{
    PVOID       BaseAddress;
    BYTE        MemoryType;
} RANGE_SEARCH_STRUCT, *PRANGE_SEARCH_STRUCT;

__forceinline
static
STATUS
IsAddressInRange(
    IN      PLIST_ENTRY                 ListEntry,
    INOUT   RANGE_SEARCH_STRUCT*        FunctionContext
    );

PTR_SUCCESS
static
MTRR_ENTRY*
MtrrTraverseFixedRegister(
    IN          QWORD           MsrRegisterValue,
    INOUT_OPT   MTRR_ENTRY*     CurrentEntry,
    IN          QWORD           RegionSize,
    INOUT       QWORD*          CurrentAddress
    );

static
STATUS
MtrrRetrieveFixedRegisters(
    void
    );

static
STATUS
MtrrRetrieveVariableRegisters(
    IN      BYTE            NumberOfVariableRegisters
    );

STATUS
MtrrInitializeRegions(
    OUT   MTRR_DATA*      MtrrData
    )
{
    STATUS status;
    QWORD msrValue;
    DWORD i;

    if( NULL == MtrrData )
    {
        return STATUS_INVALID_PARAMETER1;
    }

    if( !gGlobalData.CpuFeatures.MtrrSupport )
    {
        return STATUS_CPU_UNSUPPORTED_FEATURE;
    }

    status = STATUS_SUCCESS;

    msrValue = __readmsr( IA32_MTRRCAP );

    memcpy( &( MtrrData->Capabilities ), &msrValue, sizeof( QWORD ) );

    LOG( "Fixed register support: %d\n", MtrrData->Capabilities.FixedRegistersSupport );
    LOG( "Number of variable registers: %d\n", MtrrData->Capabilities.NumberOfVariableLengthRegisters );

    msrValue = __readmsr( IA32_MTRR_DEF_TYPE );

    MtrrData->DefaultMemoryType = msrValue & IA32_MTRR_DEFAULT_MEMORY_MASK;

    LOG( "Mtrr Enable: %d\n", IsBooleanFlagOn( msrValue, IA32_MTRR_ENABLE_MASK ) );
    LOG( "Fixed Mtrr Enable: %d\n", IsBooleanFlagOn( msrValue, IA32_FIXED_MTRR_ENABLE_MASK ) );
    LOG( "Mtrr Default Memory Type: %d\n", MtrrData->DefaultMemoryType );

    for( i = 0; i < MEMORY_TYPE_MAXIMUM_MTRR; ++i )
    {
        InitializeListHead( &( MtrrData->MtrrRegions[i] ) );
    }

    if( MtrrData->Capabilities.FixedRegistersSupport )
    {
        // retrieve fixed registers
        status = MtrrRetrieveFixedRegisters();
        if( !SUCCEEDED( status ) )
        {
            LOG( "MtrrRetrieveFixedRegisters failed with status: 0%x\n", status );
            return status;
        }
    }

    // retrieve variable length registers
    status = MtrrRetrieveVariableRegisters( ( BYTE ) MtrrData->Capabilities.NumberOfVariableLengthRegisters );
    if( !SUCCEEDED( status ) )
    {
        LOG( "MtrrRetrieveVariableRegisters failed with status: 0%x\n", status );
        return status;
    }

    return status;
}

STATUS
MtrrFindPhysicalAddressMemoryType(
    IN      PVOID           PhysicalAddress,
    OUT     BYTE*           MemoryType
    )
{
    RANGE_SEARCH_STRUCT rangeSearch;
    STATUS status;
    DWORD i;

    rangeSearch.BaseAddress = PhysicalAddress;

    for( i = 0; i < MEMORY_TYPE_MAXIMUM_MTRR; ++i )
    {
        status = ForEachElementExecute( &( gGlobalData.MtrrData.MtrrRegions[i] ), IsAddressInRange, &rangeSearch, TRUE );
        if( STATUS_ELEMENT_FOUND == status )
        {
            // we're done
            *MemoryType = rangeSearch.MemoryType;
            //LOG( "Found memory type %d for PA 0x%X\n", rangeSearch.MemoryType, rangeSearch.BaseAddress );
            return STATUS_SUCCESS;
        }
    }

    *MemoryType = gGlobalData.MtrrData.DefaultMemoryType;
    //LOG( "Using default memory type %d for PA 0x%X\n", gGlobalData.MtrrData.DefaultMemoryType, PhysicalAddress );

    return STATUS_SUCCESS;
}

static
STATUS
MtrrRetrieveFixedRegisters(
    void
    )
{
    STATUS status;
    QWORD msrValue;
    MTRR_ENTRY* pNewEntry;
    QWORD currentAddress;
    DWORD msrAddress;

    currentAddress = 0;
    pNewEntry = NULL;
    status = STATUS_SUCCESS;

    msrValue = __readmsr( IA32_MTRR_FIX64K_00000 );

    // IA32_MTRR_FIX64K_00000
    pNewEntry = MtrrTraverseFixedRegister(  msrValue,
                                            NULL,
                                            64 * KB_SIZE,
                                            &currentAddress
                                            );

    ASSERT( NULL != pNewEntry );

    msrValue = __readmsr( IA32_MTRR_FIX16K_80000 );

    // IA32_MTRR_FIX16K_80000
    pNewEntry = MtrrTraverseFixedRegister(  msrValue,
                                            pNewEntry,
                                            16 * KB_SIZE,
                                            &currentAddress
                                            );

    ASSERT( NULL != pNewEntry );

    msrValue = __readmsr( IA32_MTRR_FIX16K_A0000 );

    // IA32_MTRR_FIX16K_A0000
    pNewEntry = MtrrTraverseFixedRegister(  msrValue,
                                            pNewEntry,
                                            16 * KB_SIZE,
                                            &currentAddress
                                            );

    ASSERT( NULL != pNewEntry );

    for( msrAddress = IA32_MTRR_FIX4K_C0000; msrAddress <= IA32_MTRR_FIX4K_F8000; msrAddress = msrAddress + 1 )
    {
        msrValue = __readmsr( msrAddress );

        // IA32_MTRR_FIX4K_C0000 -> IA32_MTRR_FIX4K_F8000
        pNewEntry = MtrrTraverseFixedRegister(  msrValue,
                                                pNewEntry,
                                                4 * KB_SIZE,
                                                &currentAddress
                                                );

        ASSERT( NULL != pNewEntry );
    }


    InsertTailList( &( gGlobalData.MtrrData.MtrrRegions[pNewEntry->MemoryType] ), &( pNewEntry->ListEntry ) );

    return status;
}

static
STATUS
MtrrRetrieveVariableRegisters(
    IN      BYTE            NumberOfVariableRegisters
    )
{
    STATUS status;
    DWORD msrMtrrBaseAddr;
    DWORD msrMtrrMaskAddr;
    DWORD i;
    QWORD mtrrBase;
    QWORD mtrrMask;
    MTRR_ENTRY* pNewEntry;
    BYTE memoryType;
    QWORD baseAddress;
    QWORD endAddress;

    if( 0 == NumberOfVariableRegisters )
    {
        return STATUS_INVALID_PARAMETER2;
    }

    status = STATUS_SUCCESS;
    msrMtrrBaseAddr = 0;
    msrMtrrMaskAddr = 0;
    i = 0;
    mtrrBase = 0;
    mtrrMask = 0;
    pNewEntry = NULL;
    memoryType = MEMORY_TYPE_STRONG_UNCACHEABLE;
    baseAddress = 0;
    endAddress = 0;

    for( i = 0; i < NumberOfVariableRegisters; ++i )
    {
        msrMtrrBaseAddr = IA32_MTRR_PHYSBASE0 + 2 * i;
        msrMtrrMaskAddr = IA32_MTRR_PHYSMASK0 + 2 * i;

        mtrrMask = __readmsr( msrMtrrMaskAddr );

        if( !IsBooleanFlagOn( mtrrMask, IA32_MTRR_MASK_VALID_MASK ) )
        {
            // go to next iteration
            continue;
        }

        mtrrBase = __readmsr( msrMtrrBaseAddr );

        memoryType = mtrrBase & IA32_MTRR_BASE_MEMORY_TYPE;
        baseAddress = mtrrBase & AlignAddressLower( gGlobalData.CpuFeatures.PhysicalAddressMask, PAGE_SIZE );
        endAddress = baseAddress + ( ( ~( mtrrMask & AlignAddressLower( gGlobalData.CpuFeatures.PhysicalAddressMask, PAGE_SIZE ) ) ) & gGlobalData.CpuFeatures.PhysicalAddressMask ) + 1;

        if( NULL == pNewEntry )
        {
            pNewEntry = HvAllocPoolWithTag( PoolAllocateZeroMemory, sizeof( MTRR_ENTRY ), HEAP_MTRR_TAG, 0 );
            if( NULL == pNewEntry )
            {
                LOGL("HvAllocPoolWithTag failed\n");
                return STATUS_HEAP_INSUFFICIENT_RESOURCES;
            }

            pNewEntry->BaseAddress = baseAddress;
            pNewEntry->EndAddress = endAddress;
            pNewEntry->MemoryType = memoryType;
        }

        ASSERT(NULL != pNewEntry);
        if( pNewEntry->MemoryType == memoryType )
        {
            if( pNewEntry->EndAddress == baseAddress )
            {
                pNewEntry->EndAddress = endAddress;
                continue;
            }

            if( pNewEntry->BaseAddress == endAddress )
            {
                pNewEntry->BaseAddress = baseAddress;
                continue;
            }
        }

        // if we get here => they can't be combined
        InsertTailList( &( gGlobalData.MtrrData.MtrrRegions[pNewEntry->MemoryType] ), &( pNewEntry->ListEntry ) );

        pNewEntry = HvAllocPoolWithTag( PoolAllocateZeroMemory, sizeof( MTRR_ENTRY ), HEAP_MTRR_TAG, 0 );
        if( NULL == pNewEntry )
        {
            LOGL("HvAllocPoolWithTag failed\n");
            return STATUS_HEAP_INSUFFICIENT_RESOURCES;
        }

        pNewEntry->BaseAddress = baseAddress;
        pNewEntry->EndAddress = endAddress;
        pNewEntry->MemoryType = memoryType;

    }


    ASSERT_INFO(NULL != pNewEntry, "This LIST_ENTRY should never be NULL because NumberOfVariableRegisters is always > 0\n");
    InsertTailList( &( gGlobalData.MtrrData.MtrrRegions[pNewEntry->MemoryType] ), &( pNewEntry->ListEntry ) );


    return status;
}

PTR_SUCCESS
static
MTRR_ENTRY*
MtrrTraverseFixedRegister(
    IN          QWORD           MsrRegisterValue,
    INOUT_OPT   MTRR_ENTRY*     CurrentEntry,
    IN          QWORD           RegionSize,
    INOUT       QWORD*          CurrentAddress
    )
{
    BYTE memoryTypes[MEMORY_ENTRIES_PER_FIXED_REGISTER];
    MTRR_ENTRY* pNewEntry;
    DWORD i;

    if( ( 0 == RegionSize ) || ( NULL == CurrentAddress ) )
    {
        return NULL;
    }

    memcpy( memoryTypes, ( PVOID ) &MsrRegisterValue, sizeof( QWORD ) );

    if( NULL == CurrentEntry )
    {
        pNewEntry = HvAllocPoolWithTag( PoolAllocateZeroMemory, sizeof( MTRR_ENTRY ), HEAP_MTRR_TAG, 0 );
        if( NULL == pNewEntry )
        {
            LOGL("HvAllocPoolWithTag failed\n");
            return NULL;
        }

        pNewEntry->MemoryType = memoryTypes[0];
    }
    else
    {
        pNewEntry = CurrentEntry;
    }

    for( i = 0; i < MEMORY_ENTRIES_PER_FIXED_REGISTER; ++i )
    {
        //LOG( "[%d]Memory Type: %d\n", i, memoryTypes[i] );
        if( ( pNewEntry->MemoryType == memoryTypes[i] ) && ( pNewEntry->EndAddress == *CurrentAddress ) )
        {
            pNewEntry->EndAddress = pNewEntry->EndAddress + RegionSize;
        }
        else
        {
            // insert old entry
            InsertTailList( &( gGlobalData.MtrrData.MtrrRegions[pNewEntry->MemoryType] ), &( pNewEntry->ListEntry ) );

            // create new entry
            pNewEntry = HvAllocPoolWithTag( PoolAllocateZeroMemory, sizeof( MTRR_ENTRY ), HEAP_MTRR_TAG, 0 );
            if( NULL == pNewEntry )
            {
                LOGL("HvAllocPoolWithTag failed\n");
                return NULL;
            }

            pNewEntry->BaseAddress = *CurrentAddress;
            pNewEntry->EndAddress = pNewEntry->BaseAddress + RegionSize;
            pNewEntry->MemoryType = memoryTypes[i];
        }

        *CurrentAddress = *CurrentAddress + RegionSize;
    }

    return pNewEntry;
}

__forceinline
static
STATUS
IsAddressInRange(
    IN      PLIST_ENTRY                 ListEntry,
    INOUT   RANGE_SEARCH_STRUCT*        FunctionContext
    )
{
    MTRR_ENTRY* pEntry;

    pEntry = CONTAINING_RECORD( ListEntry, MTRR_ENTRY, ListEntry );

    // we don't use a size ATM because MTRR's are at least page multiples, and when we map guest PA we only map PAGE_SIZE
    if( ( pEntry->BaseAddress <= ( QWORD ) FunctionContext->BaseAddress ) && ( ( QWORD ) FunctionContext->BaseAddress < pEntry->EndAddress ) )
    {
        FunctionContext->MemoryType = pEntry->MemoryType;
        return STATUS_ELEMENT_FOUND;
    }

    return STATUS_SUCCESS;
}