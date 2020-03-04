#include "log.h"
#include "data.h"
#include "hv_heap.h"

typedef struct _MMU_HEAP_DATA
{
    _Guarded_by_(HeapLock)
    PHEAP_HEADER                    Heap;
    LOCK                            HeapLock;
} MMU_HEAP_DATA, *PMMU_HEAP_DATA;

static MMU_HEAP_DATA m_hvHeap;

STATUS
HvInitHeap(
    IN          PVOID                   HeapBaseAddress,
    IN          QWORD                   HeapSize
    )
{
    STATUS status;

    ASSERT(HeapBaseAddress != NULL);

    status = STATUS_SUCCESS;

    LOG("Total size reserved for heap: %U bytes ( %U KB )\n", HeapSize, HeapSize / KB_SIZE);

    status = ClHeapInit(HeapBaseAddress, HeapSize, &m_hvHeap.Heap);
    if (!SUCCEEDED(status))
    {
        LOG_FUNC_ERROR("HeapInit", status);
        return status;
    }

    LOG("HeapInit suceeded\n");

    LockInit(&m_hvHeap.HeapLock);

    return status;
}

_Always_(_When_(IsBooleanFlagOn(Flags, PoolAllocatePanicIfFail), RET_NOT_NULL))
PTR_SUCCESS
PVOID
HvAllocPoolWithTag(
    IN      DWORD                   Flags,
    IN      DWORD                   AllocationSize,
    IN      DWORD                   Tag,
    IN      DWORD                   AllocationAlignment
    )
{
    PVOID pResult;

    AcquireLock(&m_hvHeap.HeapLock);
    pResult = ClHeapAllocatePoolWithTag(
        m_hvHeap.Heap,
        Flags,
        AllocationSize,
        Tag,
        AllocationAlignment
    );
    ReleaseLock(&m_hvHeap.HeapLock);

    return pResult;
}

void
HvFreePoolWithTag(
    _Pre_notnull_ _Post_ptr_invalid_
            PVOID                   MemoryAddress,
    IN      DWORD                   Tag
    )
{
    AcquireLock(&m_hvHeap.HeapLock);
    ClHeapFreePoolWithTag(
        m_hvHeap.Heap,
        MemoryAddress,
        Tag
    );
    ReleaseLock(&m_hvHeap.HeapLock);
}