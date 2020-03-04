#include "segment.h"
#include "hv_heap.h"
#include "native/memory.h"
#include "task.h"
#include "data.h"

#define DATA_INDEX              1
#define CODE_INDEX              2

#define DATA_DESCRIPTOR         0x00CF92000000FFFFULL
#define CODE_DESCRIPTOR         0x002F9A000000FFFFULL

STATUS
CreateNewGdt(
    OUT     GDT*        NewGdt
    )
{
    STATUS status;
    SEGMENT_DESCRIPTOR* pSegments;
    QWORD descriptor;
    PCPU* pCpu;

    if( NULL == NewGdt )
    {
        return STATUS_INVALID_PARAMETER1;
    }

    status = STATUS_SUCCESS;
    pSegments = NULL;
    pCpu = GetCurrentPcpu();

    // we allocate a page so we can have up to ( 4K - 3 * 8 ) / 0x10 task descriptors = 0xFE <=> 254 CPUs
    pSegments = HvAllocPoolWithTag( PoolAllocateZeroMemory, PAGE_SIZE, HEAP_GDT_TAG, PAGE_SIZE );
    if( NULL == pSegments)
    {
        LOGL("HvAllocPoolWithTag failed\n");
        return STATUS_HEAP_INSUFFICIENT_RESOURCES;
    }

    // we copy the old data and code descriptors
    descriptor = DATA_DESCRIPTOR;
    memcpy( &pSegments[DATA_INDEX], &descriptor, sizeof( SEGMENT_DESCRIPTOR ) );

    descriptor = CODE_DESCRIPTOR;
    memcpy( &pSegments[CODE_INDEX], &descriptor, sizeof( SEGMENT_DESCRIPTOR ) );

    // set new base and limit
    NewGdt->Base = pSegments;
    NewGdt->Limit = PAGE_SIZE - 1;

    // we invalidate the GDT->Base page
    PageInvalidateTlb(NewGdt);
    PageInvalidateTlb(NewGdt->Base);

    // create new TSS descriptor
    status = CpuCreateTSSDescriptor();
    ASSERT(SUCCEEDED(status));

    // reload new GDT
    __loadGDT(gGlobalData.Gdt, CODE64_SEL, DATA64_SEL);

    // reload TR
    __loadTR(pCpu->TrSelector);

    // need to reload GS and FS
    __writemsr(IA32_GS_BASE_MSR, pCpu);
    __writemsr(IA32_FS_BASE_MSR, pCpu->VirtualCpu);

    return status;
}