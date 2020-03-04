#include "dmp_mtrr.h"
#include "log.h"
#include "data.h"

STATUS
DumpMtrrData(
    IN      MTRR_DATA*     MtrrData
    )
{
    if (MtrrData == NULL) return STATUS_INVALID_PARAMETER1;

    LOG( "MTRR List dump: \n");

    // TODO: perform implementation here

    return STATUS_SUCCESS;
}
