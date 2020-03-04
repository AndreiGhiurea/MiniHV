#include "task.h"
#include "hv_heap.h"
#include "segment.h"
#include "log.h"
#include "data.h"

//******************************************************************************
// Function:      InitializeTaskStateSegment
// Description: Sets the TSS which will be used on the current PCPU for handling
//              interrupts.
// Returns:       STATUS
// Parameter:     void
//******************************************************************************
static
STATUS
_InitializeTaskStateSegment(
    OUT_PTR PTSS*       TssAddress
    );

STATUS
CreateNewTSSDescriptor(
    IN      WORD        SelectorIndex,
    OUT_PTR PTSS*       TssAddress
    )
{
    TSS_DESCRIPTOR* pTssDescriptor;
    DWORD lowTssAddress;
    WORD midTssAdress;
    STATUS status;
    TSS* pTssAddress;

    // we need a different TSS for each PCPU
    status = _InitializeTaskStateSegment(&pTssAddress);
    ASSERT(SUCCEEDED(status));

    ASSERT(NULL != gGlobalData.Gdt);
    ASSERT(NULL != gGlobalData.Gdt->Base);

    ASSERT(NULL != TssAddress);

    lowTssAddress = QWORD_LOW(TssAddress);
    midTssAdress = DWORD_HIGH(lowTssAddress);

    pTssDescriptor = (TSS_DESCRIPTOR*)((BYTE*)gGlobalData.Gdt->Base + SelectorIndex);

    LOG("selectorIndex: 0x%x\n", SelectorIndex);

    pTssDescriptor->SegmentLimitLow = MAX_WORD;
    pTssDescriptor->BaseAddressLowWord = DWORD_LOW(lowTssAddress);
    pTssDescriptor->BaseAddressMidDword = WORD_LOW(midTssAdress);
    pTssDescriptor->Type = TSS_AVAILABLE_ENTRY64;
    pTssDescriptor->DPL = RING_ZERO_PL;
    pTssDescriptor->SegmentLimitHigh = 0xF;
    pTssDescriptor->Present = 1;
    pTssDescriptor->AVL = 0;
    pTssDescriptor->G = 0;
    pTssDescriptor->BaseAddressHighDword = WORD_HIGH(midTssAdress);
    pTssDescriptor->BaseAddressHighQword = QWORD_HIGH(TssAddress);

    // we invalidate the GDT->Base page
    /// shouldn't this be done only in case a mapping to the VA has changed?
    PageInvalidateTlb(gGlobalData.Gdt->Base);

    *TssAddress = pTssAddress;

    return STATUS_SUCCESS;
}

static
STATUS
_InitializeTaskStateSegment(
    OUT_PTR PTSS*       TssAddress
    )
{
    STATUS status;
    TSS* tssBaseAddress;

    status = STATUS_SUCCESS;
    tssBaseAddress = NULL;

    // we will use this page for the IST(we will never exceed 3900 bytes)
    tssBaseAddress = HvAllocPoolWithTag(PoolAllocateZeroMemory, PAGE_SIZE, HEAP_TASK_TAG, PAGE_SIZE);
    if (NULL == tssBaseAddress)
    {
        LOGL("HvAllocPoolWithTag failed\n");
        return STATUS_HEAP_INSUFFICIENT_RESOURCES;
    }

    // actually IST1
    tssBaseAddress->IST[IST_TO_USE - 1] = (QWORD)((BYTE*)tssBaseAddress + PAGE_SIZE);

    LOG("tssBaseAddress: 0x%X\n", tssBaseAddress);
    LOG("IST[1]: 0x%X\n", tssBaseAddress->IST[IST_TO_USE - 1]);

    // sets the TSS base address for the current PCPU
    *TssAddress = tssBaseAddress;

    return status;
}