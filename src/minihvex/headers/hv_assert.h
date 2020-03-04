#ifndef _ASSERT_H_
#define _ASSERT_H_

#include "minihv.h"

FUNC_AssertFunction         MiniHVAssert;

//******************************************************************************
// Function:    AssertHvIsStillFunctional
// Description: Issues an assert to check if the MiniHV was not halted by
//              another processor.
// Returns:       void
// Parameter:     void
//******************************************************************************
void
AssertHvIsStillFunctional(
    void
);

#endif // _ASSERT_H_