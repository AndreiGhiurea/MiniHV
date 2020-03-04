#include "dmp_minihv.h"

void
DumpMiniHvInformation(
    IN      MINIHV_INFORMATION*         MiniHvInformation
)
{
    if( NULL == MiniHvInformation )
    {
        return;
    }

    LOG("Hypervisor located at physical address: 0x%X\n", MiniHvInformation->KernelBase);
    LOG("The hypervisor occupies %d MB\n", MiniHvInformation->TotalLength / MB_SIZE );
    LOG("The hypervisor is running %s\n", MiniHvInformation->RunningNested ? "as a nested guest" : "as an L0 host");
    LOG("SIPI vector is: 0x%x\n", MiniHvInformation->TrampolineSipiVector);
}