#ifndef _VMCS_H_
#define _VMCS_H_

#include "minihv.h"
#include "cpumu.h"
#include "vmx.h"

//******************************************************************************
// Function:      __vm_preLaunch
// Description:   Initializes the GPR's and calls VMLAUNCH.
// Returns:       VMX_RESULT
// Parameter:     void
//******************************************************************************
typedef
VMX_RESULT
(__cdecl* VM_PRE_LAUNCH ) (
    IN  COMPLETE_PROCESSOR_STATE*     ProcessorState
    );

//******************************************************************************
// Function:      __vm_preResume
// Description:   Restores the GPR's from ProcessorState and calls VMRESUME.
// Returns:       VMX_RESULT
// Parameter:     IN  PROCESSOR_STATE*     ProcessorState
//******************************************************************************
typedef
VMX_RESULT
(__cdecl* VM_PRE_RESUME ) (
    IN  COMPLETE_PROCESSOR_STATE*     ProcessorState
    );


//******************************************************************************
// Function:      VmcsInitializeRegion
// Description: Calls all the necessary VM Writes.
// Returns:       STATUS
// Parameter:     void
//******************************************************************************
STATUS
VmcsInitializeRegion(
    void
    );

STATUS
VmcsReadAndWriteControls(
    IN          VMCS_FIELD              VmcsField,
    IN          QWORD                   BitsToSet,
    IN          QWORD                   BitsToClear,
    OUT_OPT     QWORD*                  InitialValue,
    OUT_OPT     QWORD*                  NewValue,
    INOUT_OPT   VMX_CAPABILITY_ARRAY*   Capabilities
);

extern VM_PRE_LAUNCH __vm_preLaunch;
extern VM_PRE_RESUME __vm_preResume;

#endif // _VMCS_H_