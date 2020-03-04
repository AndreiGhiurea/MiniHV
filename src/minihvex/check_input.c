#include "check_input.h"
#include "vmcs.h"
#include "data.h"

void
CheckSystemState(
    void 
)
{
    // we also need to check if we have imported the functions
    ASSERT( NULL != __changeStack );
    ASSERT( NULL != __loadGDT );
    ASSERT( NULL != __loadTR ); 
    
    ASSERT( NULL != __vm_preLaunch );
    ASSERT( NULL != __vm_preResume );

    CpuMuCheckFeatures();
}

void
CheckInputParameters(
    IN  ASM_PARAMETERS*     Parameters
)
{
    ASSERT( NULL != Parameters );
    ASSERT( NULL != Parameters->MultibootInformation );

    ASSERT(1 * MB_SIZE <= (QWORD) Parameters->KernelPhysicalBaseAddress);
    ASSERT(0 != Parameters->KernelSize);

    ASSERT_INFO( IsAddressAligned( Parameters->TrampolineEntryPoint, PAGE_SIZE ), "Trampoline code must be aligned on paged boundary. Address is: 0x%x\n", Parameters->TrampolineEntryPoint );
    ASSERT(1 * MB_SIZE > Parameters->TrampolineEntryPoint);

    ASSERT(NULL != Parameters->MemoryMapAddress);
    ASSERT(0 != Parameters->MemoryMapEntries);

    ASSERT(1 * MB_SIZE > Parameters->GuestPreloaderAddress);
}