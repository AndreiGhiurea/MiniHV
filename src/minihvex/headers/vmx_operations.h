#pragma once

#include "minihv.h"
#include "vmx_capability.h"
#include "bitmap.h"

// there structures are opaque and we shouldn't mess with them
typedef PVOID PVMXON_REGION;
typedef PVOID PVMCS_REGION;

// Activity states
typedef enum _ACTIVITY_STATE
{
    ActivityStateActive,
    ActivityStateHlt,
    ActivityStateShutdown,
    ActivityStateWaitForSIPI
} ACTIVITY_STATE;

typedef struct _VMX_SETTINGS
{
    volatile DWORD              CpusInVMXMode;

    volatile DWORD              CpusReceivedSIPI;

    DWORD                       GuestPreloaderAddress;
    BYTE                        GuestPreloaderStartDiskDrive;

#ifdef DEBUG
    BOOLEAN                     CheckedDummyRegisters;
    struct _REGISTER_AREA*      DummyRegisters;
#endif
} VMX_SETTINGS, *PVMX_SETTINGS;

//******************************************************************************
// Function:    VmxConfigureGlobalStructures
// Description: Retrieves the CPU capabilities, setups the EPT and the bitmaps.
// Returns:       STATUS
// Parameter:     OUT VMX_CONFIGURATION_DATA* VmxConfiguration
//******************************************************************************
STATUS
VmxConfigureGlobalStructures(
    OUT     VMX_CONFIGURATION_DATA*         VmxConfiguration,
    OUT     VMX_SETTINGS*                   VmxSettings
);


//******************************************************************************
// Function:      VmxSetupCpu
// Description: Checks for valid CR4 and IA32_FEATURE_CONTROL values and sets
//              them appropriately (if IA32_FEATURE_CONTROL is not locked).
// Returns:       STATUS
// Parameter:     void
//******************************************************************************
STATUS
VmxCheckAndSetupCpu(
    void
    );

//******************************************************************************
// Function:      VmxStartVmxOn
// Description: Writes identifier to VMXON region and starts VMX operation.
// Returns:       STATUS
// Parameter:     void
//******************************************************************************
STATUS
VmxStartVmxOn(
    void
    );

//******************************************************************************
// Function:      VmxSetupVmcsStructures
// Description: Writes revision identifier, clears VMCS, loads pointer and does
//              all the *annoying* VMWrites.
// Returns:       STATUS
// Parameter:     IN VMX_CONFIGURATION_DATA* VmxCapabilities
//******************************************************************************
STATUS
VmxSetupVmcsStructures(
    IN      VMX_CONFIGURATION_DATA*         VmxCapabilities
    );

//******************************************************************************
// Function:      VmxStartGuest
// Description: Launches the guest.
// Returns:       STATUS
// Parameter:     void
//******************************************************************************
STATUS
VmxStartGuest(
    void
    );

//******************************************************************************
// Function:    VmxResumeGuest
// Description: Resume guest execution. Checks to see if there is no more than
//              1 pending event to be injected into the guest.
// Returns:       void
// Parameter:     void
//******************************************************************************
void
VmxResumeGuest(
    void
);

STATUS
VmxSetActivityState(
    IN      ACTIVITY_STATE      ActivityState
    );
