#ifndef _LOCK_H_
#define _LOCK_H_

#include "minihv.h"
#include "lock_common.h"

//******************************************************************************
// Function:      AcquireLock
// Description: Acquires lock. If the MONITOR feature is available it will be
//              used. Else busy waiting will occur.
// Returns:       void - The lock will be held by the processor when the function
//                     returns
// Parameter:     IN PLOCK Lock
//******************************************************************************
void
AcquireLock(
    INOUT   PLOCK      Lock
);

//******************************************************************************
// Function:      ReleaseLock
// Description: Releases held lock.
// Returns:       void
// Parameter:     IN PLOCK Lock
//******************************************************************************
void
ReleaseLock(
    INOUT    PLOCK      Lock              
);

#endif // _LOCK_H_
