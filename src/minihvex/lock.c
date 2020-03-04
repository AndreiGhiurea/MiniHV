#include "lock.h"
#include "cpumu.h"
#include "log.h"
#include "data.h"

void
AcquireLock(
    INOUT   PLOCK      Lock
    )
{
    INTR_STATE dummy;

    LockAcquire(Lock, &dummy );
}

void
ReleaseLock(
    INOUT   PLOCK      Lock             
)
{
    LockRelease(Lock, INTR_OFF );
}