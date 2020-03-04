#include "acpi_interface.h"
#include "dmp_apic.h"
#include "log.h"
#include "cpumu.h"
#include "hv_heap.h"
#include "native/memory.h"
#include "data.h"

void
ApicRetrievePhysCpu(
    INOUT     LIST_ENTRY*     PhysicalCpuList
    )
{
    ACPI_TABLE_HEADER* table;
    ACPI_STATUS acpiStatus;
    ACPI_TABLE_DESC madtTable;
    DWORD actualTableLength;
    DWORD offsetInTable;
    PVOID pStackBottom;
    ACPI_MADT_LOCAL_APIC* pProc;
    BYTE* pData;
    PCPU* pCpu;
    DWORD apicId;

    ASSERT( NULL != PhysicalCpuList );

    acpiStatus = AcpiInitializeTables( &madtTable, 1, TRUE );
    ASSERT( AE_OK == acpiStatus );

    acpiStatus = AcpiGetTable( ACPI_SIG_MADT, 1, &table );
    ASSERT( AE_OK == acpiStatus );

    // we need to see which one is the BSP and mark it
    apicId = CpuGetApicId();

    pStackBottom = NULL;
    offsetInTable = 0;
    actualTableLength = table->Length - sizeof( ACPI_TABLE_MADT );
    pData = ( BYTE* ) table + sizeof( ACPI_TABLE_MADT );
    while( offsetInTable < actualTableLength )
    {
        pCpu = NULL;
        pProc = ( ACPI_MADT_LOCAL_APIC* ) &(pData[offsetInTable]);
        if( ACPI_MADT_TYPE_LOCAL_APIC == pProc->Header.Type )
        {
            if( pProc->LapicFlags & APIC_PROCESSOR_ACTIVE )
            {
                pCpu = ( PCPU* ) HvAllocPoolWithTag( PoolAllocatePanicIfFail | PoolAllocateZeroMemory, sizeof( PCPU ), HEAP_GLOBAL_TAG, 0 );

                // no need to check the result
                pCpu->ApicID = pProc->Id;

                pStackBottom = HvAllocPoolWithTag(PoolAllocatePanicIfFail | PoolAllocateZeroMemory, STACK_SIZE, HEAP_CPU_TAG, PAGE_SIZE);

                pCpu->StackBase = ( PVOID )( ( BYTE* ) pStackBottom + STACK_SIZE );

                // we unmap the last page of the stack so we can detect stack corruptions
                ASSERT(SUCCEEDED(UnmapMemory( pStackBottom, (STACK_SIZE - EFFECTIVE_STACK_SIZE))));

                pCpu->VirtualCpu = ( VCPU* ) HvAllocPoolWithTag( PoolAllocatePanicIfFail | PoolAllocateZeroMemory, sizeof( VCPU ), HEAP_VMX_TAG, PAGE_SIZE );

                pCpu->VirtualCpu->PhysicalCpu = pCpu;
                pCpu->VirtualCpu->VmcsRegion = ( PVMCS_REGION ) HvAllocPoolWithTag( PoolAllocatePanicIfFail | PoolAllocateZeroMemory, gGlobalData.VmxConfigurationData.VmcsRegionSize, HEAP_VMX_TAG, PAGE_SIZE );

                pCpu->VirtualCpu->WaitingForWakeup = TRUE;

                pCpu->VmxOnRegion = ( PVMXON_REGION ) HvAllocPoolWithTag( PoolAllocatePanicIfFail | PoolAllocateZeroMemory, gGlobalData.VmxConfigurationData.VmcsRegionSize, HEAP_VMX_TAG, PAGE_SIZE );

                // we should only write IA32_FS_BASE_MSR with pCpu->VirtualCpu after we have actually allocated it :)
                if( apicId == pProc->Id )
                {
                    pCpu->BspProcessor = TRUE;

                    __writemsr(IA32_GS_BASE_MSR, pCpu);
                    __writemsr(IA32_FS_BASE_MSR, pCpu->VirtualCpu);
                }

                InsertTailList( PhysicalCpuList, &( pCpu->ListEntry ) );
            }
        }

        offsetInTable = offsetInTable + pProc->Header.Length;
    }

}