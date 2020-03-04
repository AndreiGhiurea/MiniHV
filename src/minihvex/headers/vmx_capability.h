#ifndef _VMX_CAPABILITY_H_
#define _VMX_CAPABILITY_H_

#include "minihv.h"
#include "ept.h"
#include "lock.h"

#define NO_OF_CONTROLS                      32

#define HLT_ACTIVITY_SUPPORT                (1<<0)
#define SHUTDOWN_ACTIVYT_SUPPORT            (1<<1)
#define WAIT_FOR_SIPI_ACTIVITY_SUPPORT      (1<<2)


#define VMX_CAPABILITIES_PINBASED           0
#define VMX_CAPABILITIES_PROCBASED          1
#define VMX_CAPABILITIES_EXIT               2
#define VMX_CAPABILITIES_ENTRY              3
#define VMX_CAPABILITIES_SEC_PROCBASED      4
#define VMX_CAPABILITIES_CR0                5
#define VMX_CAPABILITIES_CR4                6

#define VMX_TRUE_REGULAR_DIFFERENCE         ( IA32_VMX_TRUE_PINBASED_CTLS - IA32_VMX_PINBASED_CTLS )

#define EPT_EXECUTE_ONLY_SUPPORT            (1<<0)
#define EPT_PAGE_WALK_4_SUPPORT             (1<<6)
#define EPT_UC_MEMORY_SUPPORT               (1<<8)
#define EPT_WB_MEMORY_SUPPORT               (1<<14)

#pragma pack(push,1)
typedef struct _VMX_CAPABILITY_ARRAY
{
    DWORD                   AllowedZeroSetting;
    DWORD                   AllowedOneSetting;
    DWORD                   DefaultZeroClass;
    DWORD                   FlexibleValue;
    DWORD                   ChosenValue;
} VMX_CAPABILITY_ARRAY, *PVMX_CAPABILITY_ARRAY;

typedef struct _EPT_SUPPORT
{
    // If TRUE other fields are relevant, else no EPT's :(
    BOOLEAN                 SupportAvailable;

    // can map execute-only memory
    BOOLEAN                 ExecuteOnly;

    // support for page-walk length of 4
    BOOLEAN                 PageWalkLength4;

    // Memory type to use for EPT paging structures
    BYTE                    MemoryType;
} EPT_SUPPORT, *PEPT_SUPPORT;


#pragma warning(push)

//warning C4214: nonstandard extension used : bit field types other than int
#pragma warning(disable:4214)
typedef struct _VMX_MISC_OPTIONS
{
    // 4:0
    QWORD                   VMXRateTSC                  :   5;

    // 5 - If set => VM Exits store IA32_EFER.LMA into
    // "IA-32e mode guest" VM-entry control
    QWORD                   Ia32eAutoStore              :   1;

    // 8:6 Activity State supported
    QWORD                   HltActivityState            :   1;
    QWORD                   ShutdownActivityState       :   1;
    QWORD                   WaitForSipiActivityState    :   1;

    // 14:9 Reserved
    QWORD                   Reserved0                   :   6;

    // 15 - If set => RDMSR[IA32_SMBASE] can be used in SMM
    QWORD                   RdmsrSMM                    :   1;

    // 24:16 Number of CR3-target values supported by the processor
    QWORD                   Cr3MaxValues                :   9;

    // 27:25 recommended maximum of MSR's in VM-exit MSR store/load lists
    // Formula MaxMSRS = 512 * ( MaxMsrs + 1 )
    QWORD                   MaxMsrs                     :   3;

    // 28 - if set => bit 2 of IA32_SMM_MONITOR_CTL can be set to 1
    QWORD                   SmmMonitorEnable            :   1;

    // 29 - if set => VMWRITE to any field
    QWORD                   VmWriteGlobal               :   1;

    // 63:32 MSEG identifier
    QWORD                   MSEGIdentifier              :  32;
} VMX_MISC_OPTIONS, *PVMX_MISC_OPTIONS;
STATIC_ASSERT( sizeof( VMX_MISC_OPTIONS ) == sizeof( QWORD ) );
#pragma warning(pop)

typedef struct _VMX_MSR_LOAD_ENTRY
{
    QWORD                   MsrIndex;
    QWORD                   MsrValue;
} VMX_MSR_LOAD_ENTRY, *PVMX_MSR_LOAD_ENTRY;
STATIC_ASSERT( sizeof( VMX_MSR_LOAD_ENTRY ) == 2 * sizeof( QWORD ) );

// This structure contains the configuration supported by the processor
typedef struct _VMX_CONFIGURATION_DATA
{
    // IA32_VMX_BASIC_MSR[30:0]
    DWORD                       VmcsRevisionIdentifier;

    // Number of bytes to allocate for VMCS and VMXON region
    // IA32_VMX_BASIC_MSR[44:32]
    DWORD                       VmcsRegionSize;

    // See IA32_VMX_BASIC_MSR for values
    BYTE                        MemoryTypeSupported;

    // if set we can read the VMX_TRUE_*_CTLS MSRs
    BOOLEAN                     VmxTrueControlsSupported;

    // if set the processor reports information in the VM-exit
    // instruction-information field on exits due to INS/OUTS
    BOOLEAN                     InsOutExitInformation;

    VMX_CAPABILITY_ARRAY        PinBasedControls;
    VMX_CAPABILITY_ARRAY        PrimaryProcessorBasedControls;
    VMX_CAPABILITY_ARRAY        SecondaryProcessorBasedControls;
    VMX_CAPABILITY_ARRAY        VmxExitControls;
    VMX_CAPABILITY_ARRAY        VmxEntryControls;

    VMX_CAPABILITY_ARRAY        Cr0Values;
    VMX_CAPABILITY_ARRAY        Cr4Values;

    EPT_SUPPORT                 EptSupport;

    VMX_MISC_OPTIONS            MiscOptions;
} VMX_CONFIGURATION_DATA, *PVMX_CONFIGURATION_DATA;

#pragma pack(pop)

STATUS
VmxCapabilityRetrieveInformation(
    OUT     VMX_CAPABILITY_ARRAY*   Capabilities,
    IN      DWORD                   CapabilityIndex,
    IN      BOOLEAN                 TrueControls
    );

STATUS
VmxWriteControlsAfterCheckingCapabilities(
    IN      DWORD                   VmcsField,
    INOUT   VMX_CAPABILITY_ARRAY*   Capabilities,
    INOUT   DWORD*                  ControlValues
);

STATUS
VmxCapabilityCheckEptSupport(
    OUT     EPT_SUPPORT*            EptSupport
    );

STATUS
VmxCapabilityRetrieveAditionalOptions(
    OUT     VMX_MISC_OPTIONS*       MiscOptions
    );

#endif // _VMX_CAPABILITY_H_