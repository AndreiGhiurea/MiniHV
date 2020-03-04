#pragma once

#include "cl_heap.h"

#define HEAP_TEST_TAG                   ':TST'
#define HEAP_ACPI_TAG                   'IPCA'
#define HEAP_GLOBAL_TAG                 ':BLG'
#define HEAP_IDT_TAG                    ':TDI'
#define HEAP_PAGE_TAG                   'EGAP'
#define HEAP_CPU_TAG                    ':UPC'
#define HEAP_VMX_TAG                    ':XMV'
#define HEAP_TASK_TAG                   'KSAT'
#define HEAP_GDT_TAG                    ':TDG'
#define HEAP_EPT_TAG                    ':TPE'
#define HEAP_MTRR_TAG                   'RRTM'
#define HEAP_PCI_TAG                    ':ICP'
#define HEAP_COMM_TAG                   ':MOC'
#define HEAP_INT15_TAG                  ':TNI'
#define HEAP_IPC_TAG                    ':CPI'
#define HEAP_GST_CB_TAG                 ':BCG'

STATUS
HvInitHeap(
    IN          PVOID                   HeapBaseAddress,
    IN          QWORD                   HeapSize
    );

_Always_(_When_(IsBooleanFlagOn(Flags, PoolAllocatePanicIfFail), RET_NOT_NULL))
PTR_SUCCESS
PVOID
HvAllocPoolWithTag(
    IN      DWORD                   Flags,
    IN      DWORD                   AllocationSize,
    IN      DWORD                   Tag,
    IN      DWORD                   AllocationAlignment
    );

void
HvFreePoolWithTag(
    _Pre_notnull_ _Post_ptr_invalid_
            PVOID                   MemoryAddress,
    IN      DWORD                   Tag
    );
