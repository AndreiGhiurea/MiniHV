#ifndef _CHECK_INPUT_H_
#define _CHECK_INPUT_H_

#include "minihv.h"
#include "multiboot.h"

// 
// 
//******************************************************************************
// Function:      CheckSystemState
// Description: This function checks that the hardware-defined structures are
//              sized properly and if all the assembly functions are imported
//              If anything fails an ASSERT will be triggered.
// Returns:       void
// Parameter:     void
//******************************************************************************
void
CheckSystemState(
    void 
);

//******************************************************************************
// Function:      CheckInputParameters
// Description: Checks the YASM parameters received.
// Returns:       void
// Parameter:     IN ASM_PARAMETERS* Parameters - Parameters received
//******************************************************************************
void
CheckInputParameters(
    IN  ASM_PARAMETERS*     Parameters
);

#endif // _CHECK_INPUT_H_