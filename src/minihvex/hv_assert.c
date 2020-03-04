#include "hv_assert.h"
#include "display.h"
#include "serial.h"
#include "log.h"
#include "data.h"


#pragma warning(push)
// 'SystemPanic' : recursive on all control paths, function will cause runtime stack overflow
#pragma warning(disable:4717)

void
(__cdecl MiniHVAssert)(
    IN_Z    char*           Message
)
{
    // set the halt flag
    if( !IsBooleanFlagOn( _InterlockedOr(&gGlobalData.MiniHvInformation.MiniHvFlags, MINIHV_FLAGS_HALTED), MINIHV_FLAGS_HALTED ) )
    {
        // if we're here it means that we're the first to HALT the processor (we must signal the other processors)
        if (NULL != gGlobalData.ApicData.ApicBaseAddress)
        {
            // it means we have woken up other CPUs
            LOGP("Sending IPI\n");
            ApicSendIpi(0, ApicDeliveryModeINIT, ApicDestinationShorthandAllExcludingSelf, NULL);
        }
    }

    if( NULL != Message )
    {
        // write the message
        if( 0 != gGlobalData.SerialPortNumber )
        {
            BOOLEAN needToAcquireLock = !LockIsOwner(&gGlobalData.LogData.SerialLock);

            if (needToAcquireLock)
            {
                // we may have already have taken the lock when we 'panicked'
                AcquireLock(&gGlobalData.LogData.SerialLock);
            }
            SerialWriteBuffer( Message );

            // no matter what we want to release the lock
            ReleaseLock(&gGlobalData.LogData.SerialLock);
        }

        perror( Message );
    }

    // release assert lock
    AssertFreeLock();

    // the VmExitHandler is called with interrupts disabled
    // and we NEVER re-enable them
    ASSERT( !IsBooleanFlagOn(__readeflags(), RFLAGS_INTERRUPT_FLAG_BIT ) )

    // HLT
    __halt();

    // WE WILL NEVER EVER get here
    NOT_REACHED;
}
#pragma warning(pop)

void
AssertHvIsStillFunctional(
    void
)
{
    if( MINIHV_FLAGS_HALTED == _InterlockedAnd( &gGlobalData.MiniHvInformation.MiniHvFlags, MAX_DWORD ) )
    {
        ASSERT_INFO( FALSE, "MiniHV was halted\n");
    }
}