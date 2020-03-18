#include "dmp_mtrr.h"
#include "log.h"
#include "data.h"

STATUS
DumpMtrrData(
    _In_      MTRR_DATA*     MtrrData
)
{
    if (MtrrData == NULL) return STATUS_INVALID_PARAMETER1;

    PLIST_ENTRY pList = NULL;
    PMTRR_ENTRY pEntry = NULL;

    LOG("MTRR List dump: \n");

    for (DWORD i = 0; i < MtrrData->NumberOfListEntries; i++)
    {
        pList = &(MtrrData->MtrrRegions[i]);
        while (pList)
        {
            pEntry = CONTAINING_RECORD(pList, MTRR_ENTRY, ListEntry);
            pList = pList->Flink;

            LOGPL("Base: 0x%016x; End: 0x%016x; Type: 0x%x\n", pEntry->BaseAddress, pEntry->EndAddress, pEntry->MemoryType);
        }
    }

    return STATUS_SUCCESS;
}
