#include "minihv.h"
#include "dmp_common.h"
#include "rec_rw_spinlock.h"

static REC_RW_SPINLOCK m_dumpLock;

void
DumpPreinit(
    void
)
{
    RecRwSpinlockInit(2, &m_dumpLock);
}

REQUIRES_NOT_HELD_LOCK(m_dumpLock)
RELEASES_EXCL_AND_REENTRANT_LOCK(m_dumpLock)
INTR_STATE
DumpTakeLock(
    void
)
{
    INTR_STATE oldState;

    RecRwSpinlockAcquireExclusive(&m_dumpLock, &oldState);

    return oldState;
}

REQUIRES_EXCL_LOCK(m_dumpLock)
RELEASES_EXCL_AND_REENTRANT_LOCK(m_dumpLock)
void
DumpReleaseLock(
    IN INTR_STATE   OldIntrState
)
{
    RecRwSpinlockReleaseExclusive(&m_dumpLock, OldIntrState);
}