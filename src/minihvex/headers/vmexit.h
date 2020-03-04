#ifndef _VMEXIT_H_
#define _VMEXIT_H_

#include "minihv.h"
#include "cpumu.h"
#include "vmexit_reason.h"
#include "vmx_exit.h"

//******************************************************************************
// Function:      VmExitHandler
// Description: Function called from YASM after guest GPR's are saved in
//              ProcessorState.
// Returns:       void
// Parameter:     INOUT PROCESSOR_STATE * ProcessorState - State of guest GPRs
//******************************************************************************
void
VmExitHandler(
    INOUT     COMPLETE_PROCESSOR_STATE*        ProcessorState
    );

#endif // _VMEXIT_H_