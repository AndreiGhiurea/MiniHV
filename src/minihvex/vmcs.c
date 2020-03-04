#include "vmcs.h"
#include "vmguest.h"
#include "vmexit.h"
#include "log.h"
#include "idt.h"
#include "vmx_capability.h"
#include "data.h"

#pragma warning(push)

// warning C28039: The type of actual parameter '24576|2048|((0<<1))|0' should exactly match the type 'VMCS_FIELD':
#pragma warning(disable: 28039)

// this will be registered as the host RIP so
// we can restore GPRs before calling the actual
// C handler
extern void VmPreexitHandler();

//******************************************************************************
// Function:      VmcsInitializeGuestState
// Description: Initializes Guest State
// Returns:       STATUS
// Parameter:     void
//******************************************************************************
static
STATUS
VmcsInitializeGuestState(
    void
    );

//******************************************************************************
// Function:      VmcsInitializeHostState
// Description: Initializes Host State
// Returns:       STATUS
// Parameter:     void
//******************************************************************************
static
STATUS
VmcsInitializeHostState(
    void
    );

//******************************************************************************
// Function:      VmcsInitializeExecutionControlFields
// Description: Initializes Control Fields
// Returns:       STATUS
// Parameter:     void
//******************************************************************************
static
STATUS
VmcsInitializeVMXControls(
    void
    );

STATUS
VmcsInitializeRegion(
    void
    )
{
    STATUS status;
    PCPU* physCpu;

    status = STATUS_SUCCESS;
    physCpu = GetCurrentPcpu();

    // VMX controls should be the first ones
    // this is because this is where the chosen value is set
    // for the capabilities
    status = VmcsInitializeVMXControls();
    ASSERT(SUCCEEDED(status));

    status = VmcsInitializeGuestState();
    ASSERT( SUCCEEDED( status ) );

    status = VmcsInitializeHostState();
    ASSERT( SUCCEEDED( status ) );

    return status;
}

STATUS
VmcsReadAndWriteControls(
    IN          VMCS_FIELD              VmcsField,
    IN          QWORD                   BitsToSet,
    IN          QWORD                   BitsToClear,
    OUT_OPT     QWORD*                  InitialValue,
    OUT_OPT     QWORD*                  NewValue,
    INOUT_OPT   VMX_CAPABILITY_ARRAY*   Capabilities
)
{
    QWORD vmcsValue;
    STATUS status;

    status = STATUS_SUCCESS;

    vmcsValue = VmxRead(VmcsField);

    if( NULL != InitialValue )
    {
        *InitialValue = vmcsValue;
    }

    if( 0 != BitsToClear )
    {
        vmcsValue = vmcsValue & (~BitsToClear);
    }

    if( 0 != BitsToSet )
    {
        vmcsValue = vmcsValue | BitsToSet;
    }

    if( NULL != Capabilities )
    {
        status = VmxWriteControlsAfterCheckingCapabilities((DWORD)VmcsField, Capabilities, (DWORD*) &vmcsValue);
        if( !SUCCEEDED( status ) )
        {
            return status;
        }
    }
    else
    {
        VmxWrite(VmcsField, vmcsValue);
    }

    if( NULL != NewValue )
    {
        *NewValue = vmcsValue;
    }

    return STATUS_SUCCESS;
}

static
STATUS
VmcsInitializeGuestState(
    void
    )
{
    QWORD ia32PatValues;
    DWORD tempValue;
    STATUS status;
    ACTIVITY_STATE activityState;

    // 24.4 Guest State Area

    // 24.4.1 Guest Register State

#pragma region Guest Register State
    // CR0
    tempValue = VM_GUEST_CR0;
    LOG( "About to check Guest CR0 values\n" );
    status = VmxWriteControlsAfterCheckingCapabilities( VMCS_GUEST_CR0, &(gGlobalData.VmxConfigurationData.Cr0Values), &tempValue );
    ASSERT( SUCCEEDED( status ) );

    // CR3
    VmxWrite( VMCS_GUEST_CR3, VM_GUEST_CR3 );

    // CR4
    tempValue = VM_GUEST_CR4;
    LOG( "About to check Guest CR4 values\n" );
    status = VmxWriteControlsAfterCheckingCapabilities( VMCS_GUEST_CR4, &(gGlobalData.VmxConfigurationData.Cr4Values), &tempValue );
    ASSERT( SUCCEEDED( status ) );

    // DR7
    VmxWrite( VMCS_GUEST_DR7, VM_GUEST_DR7 );

    // RSP
    VmxWrite( VMCS_GUEST_RSP, VM_GUEST_RSP );

    // RIP
    VmxWrite( VMCS_GUEST_RIP, gGlobalData.VmxCurrentSettings.GuestPreloaderAddress );

    // RFLAGS
    VmxWrite( VMCS_GUEST_RFLAGS, VM_GUEST_RFLAGS );

    // CS
    // CS.selector
    VmxWrite( VMCS_GUEST_CS_SELECTOR, VM_GUEST_CS_SELECTOR );

    // CS.address
    VmxWrite( VMCS_GUEST_CS_BASE, VM_GUEST_CS_ADDRESS );

    // CS.Limit
    VmxWrite( VMCS_GUEST_CS_LIMIT, VM_GUEST_CS_SEG_LIMIT );

    // CS.AccessRights
    VmxWrite( VMCS_GUEST_CS_ACCESS_RIGHTS, VM_GUEST_CS_ACCESS_RIGHTS );

    // SS
    // SS.selector
    VmxWrite(VMCS_GUEST_SS_SELECTOR, VM_GUEST_SS_SELECTOR);

    // SS.address
    VmxWrite( VMCS_GUEST_SS_BASE, VM_GUEST_SS_ADDRESS );

    // SS.Limit
    VmxWrite( VMCS_GUEST_SS_LIMIT, VM_GUEST_SS_SEG_LIMIT );

    // SS.AccessRights
    VmxWrite( VMCS_GUEST_SS_ACCESS_RIGHTS, VM_GUEST_SS_ACCESS_RIGHTS );

    // DS
    // DS.selector
    VmxWrite( VMCS_GUEST_DS_SELECTOR, VM_GUEST_DS_SELECTOR );

    // DS.address
    VmxWrite( VMCS_GUEST_DS_BASE, VM_GUEST_DS_ADDRESS );

    // DS.Limit
    VmxWrite( VMCS_GUEST_DS_LIMIT, VM_GUEST_DS_SEG_LIMIT );

    // DS.AccessRights
    VmxWrite( VMCS_GUEST_DS_ACCESS_RIGHTS, VM_GUEST_DS_ACCESS_RIGHTS );

    // ES
    // ES.selector
    VmxWrite( VMCS_GUEST_ES_SELECTOR, VM_GUEST_ES_SELECTOR );

    // ES.address
    VmxWrite(VMCS_GUEST_ES_BASE, VM_GUEST_ES_ADDRESS);

    // ES.Limit
    VmxWrite( VMCS_GUEST_ES_LIMIT, VM_GUEST_ES_SEG_LIMIT );

    // ES.AccessRights
    VmxWrite( VMCS_GUEST_ES_ACCESS_RIGHTS, VM_GUEST_ES_ACCESS_RIGHTS );

    // FS
    // FS.selector
    VmxWrite(VMCS_GUEST_FS_SELECTOR, VM_GUEST_FS_SELECTOR);

    // FS.address
    VmxWrite( VMCS_GUEST_FS_BASE, VM_GUEST_FS_ADDRESS );

    // FS.Limit
    VmxWrite( VMCS_GUEST_FS_LIMIT, VM_GUEST_FS_SEG_LIMIT );

    // FS.AccessRights
    VmxWrite( VMCS_GUEST_FS_ACCESS_RIGHTS, VM_GUEST_FS_ACCESS_RIGHTS );

    // GS
    // GS.selector
    VmxWrite( VMCS_GUEST_GS_SELECTOR, VM_GUEST_GS_SELECTOR );

    // GS.address
    VmxWrite( VMCS_GUEST_GS_BASE, VM_GUEST_GS_ADDRESS );

    // GS.Limit
    VmxWrite( VMCS_GUEST_GS_LIMIT, VM_GUEST_GS_SEG_LIMIT );

    // GS.AccessRights
    VmxWrite( VMCS_GUEST_GS_ACCESS_RIGHTS, VM_GUEST_GS_ACCESS_RIGHTS );

    // LDTR
    // LDTR.selector
    VmxWrite( VMCS_GUEST_LDTR_SELECTOR, VM_GUEST_LDTR_SELECTOR );

    // LDTR.address
    VmxWrite( VMCS_GUEST_LDTR_BASE, VM_GUEST_LDTR_ADDRESS );

    // LDTR.Limit
    VmxWrite( VMCS_GUEST_LDTR_LIMIT, VM_GUEST_LDTR_SEG_LIMIT );

    // LDTR.AccessRights
    VmxWrite( VMCS_GUEST_LDTR_ACCESS_RIGHTS, VM_GUEST_LDTR_ACCESS_RIGHTS );

    // TR
    // TR.selector
    VmxWrite( VMCS_GUEST_TR_SELECTOR, VM_GUEST_TR_SELECTOR );

    // TR.address
    VmxWrite( VMCS_GUEST_TR_BASE, VM_GUEST_TR_ADDRESS );

    // TR.Limit
    VmxWrite( VMCS_GUEST_TR_LIMIT, VM_GUEST_TR_SEG_LIMIT );

    // TR.AccessRights
    VmxWrite( VMCS_GUEST_TR_ACCESS_RIGHTS, VM_GUEST_TR_ACCESS_RIGHTS );

    // GDTR
    // GDTR.address
    VmxWrite( VMCS_GUEST_GDTR_BASE, VM_GUEST_GDTR_BASE_ADDRESS );

    // GDTR.Limit
    VmxWrite( VMCS_GUEST_GDTR_LIMIT, VM_GUEST_GDTR_LIMIT );

    // IDTR
    // IDTR.address
    VmxWrite( VMCS_GUEST_IDTR_BASE, VM_GUEST_IDTR_BASE_ADDRESS );

    // IDTR.Limit
    VmxWrite( VMCS_GUEST_IDTR_LIMIT, VM_GUEST_IDTR_LIMIT );

    // IA32_DEBUGCTL
    VmxWrite( VMCS_GUEST_IA32_DEBUGCTL_FULL, VM_GUEST_IA32_DEBUGCTL );

    // IA32_SYSENTER_CS
    VmxWrite( VMCS_GUEST_IA32_SYSENTER_CS, VM_GUEST_IA32_SYSENTER_CS );

    // IA32_SYSENTER_ESP
    VmxWrite( VMCS_GUEST_IA32_SYSENTER_ESP, VM_GUEST_IA32_SYSENTER_ESP );

    // IA32_SYSENTER_EIP
    VmxWrite( VMCS_GUEST_IA32_SYSENTER_EIP, VM_GUEST_IA32_SYSENTER_EIP );

    // Needs only to be set if
    // "load IA32_PERF_GLOBAL_CTRL" is set
    VmxWrite( VMCS_GUEST_IA32_PERF_GLOBAL_CTRL_FULL, VM_GUEST_IA32_PERF_GLOBAL_CTRL );

    // Only if "load IA32_PAT"
    if (IsBooleanFlagOn(gGlobalData.VmxConfigurationData.VmxEntryControls.AllowedOneSetting, ENTRY_CONTROL_LOAD_IA32_PAT) ||
        IsBooleanFlagOn(gGlobalData.VmxConfigurationData.VmxExitControls.AllowedOneSetting, EXIT_CONTROL_SAVE_IA32_PAT))
    {
        ia32PatValues = __readmsr(IA32_PAT);
        VmxWrite(VMCS_GUEST_IA32_PAT_FULL, ia32PatValues);
    }

    // Only if "load IA32_EFER" or "save IA32_EFER" is set
    VmxWrite( VMCS_GUEST_IA32_EFER_FULL, VM_GUEST_IA32_EFER );

    // SMBASE
    VmxWrite( VMCS_GUEST_SMBASE, VM_GUEST_SMBASE );
#pragma endregion

    // 24.4.2 Guest Non-Register State

#pragma region Guest Non-Register State

    // Guest Activity State
    if( GetCurrentPcpu()->BspProcessor )
    {
        activityState = VM_GUEST_ACTIVITY_STATE_BSP;
    }
    else
    {
        if (gGlobalData.MiniHvInformation.RunningNested)
        {
            /// because VMWare is incapable of implementing wait-for SIPI properly we will use INITs instead of SIPI
            /// messages to wake up the APs (SIPI exit qualification is not populated with the start vector)
            // we need to start in the HLT state because if we're in the wait for SIPI state we cannot receive INIT messages
            // (used as the communication mechanism between CPUs)
            activityState = VM_GUEST_ACTIVITY_STATE_AP_NESTED;
        }
        else if (IsBooleanFlagOn(gGlobalData.VmxConfigurationData.PinBasedControls.ChosenValue, PIN_BASED_ACTIVATE_PREEMPT_TIMER))
        {
            activityState = VM_GUEST_ACTIVITY_STATE_AP_WITH_TIMER;
        }
        else
        {
            activityState = VM_GUEST_ACTIVITY_STATE_AP_NO_TIMER;
        }
    }
    status = VmxSetActivityState(activityState);
    ASSERT( SUCCEEDED(status));

    // Guest Interruptibility State
    VmxWrite( VMCS_GUEST_INT_STATE, VM_GUEST_INTERRUPTIBILITY_STATE );

    // Guest Pending Debug Exception
    VmxWrite( VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS, VM_GUEST_PENDING_DEBUG_EXCEPTION );

    // if "VMCS shadowing" is set =>
    // VMREAD and VMWRITE access the VMCS referenced by this pointer
    // should be set to 0xFFFFFFFF_FFFFFFFF if clear
    VmxWrite( VMCS_GUEST_VMCS_LINK_POINTER_FULL, VM_GUEST_VMCS_LINK_POINTER );

    if (IsBooleanFlagOn(gGlobalData.VmxConfigurationData.PinBasedControls.ChosenValue, PIN_BASED_ACTIVATE_PREEMPT_TIMER))
    {
        // if "activate VMX-preemption timer" is set
        LOG("We are using the preemption timer\n");
    }
    else
    {
        LOG("We are not using the preemption timer :(\n");
    }

    // if "enable EPT" and PAE paging is used
    VmxWrite( VMCS_GUEST_PDPTE0_FULL, VM_GUEST_PDPTE_S );
    VmxWrite( VMCS_GUEST_PDPTE1_FULL, VM_GUEST_PDPTE_S );
    VmxWrite( VMCS_GUEST_PDPTE2_FULL, VM_GUEST_PDPTE_S );
    VmxWrite( VMCS_GUEST_PDPTE3_FULL, VM_GUEST_PDPTE_S );

#pragma endregion


    return STATUS_SUCCESS;
}

static
STATUS
VmcsInitializeHostState(
    void
    )
{
    PCPU* pCpu;

    pCpu = GetCurrentPcpu();
    ASSERT( NULL != pCpu );

    // 24.5 Host State Area

    // CR0
    VmxWrite( VMCS_HOST_CR0, __readcr0() );

    // CR3
    VmxWrite( VMCS_HOST_CR3, (QWORD) __readcr3() );

    // CR4
    VmxWrite( VMCS_HOST_CR4, __readcr4() );

    // RSP
    // this should probably be set exactly before vmlaunch
    // and set differently for each CPU
    VmxWrite( VMCS_HOST_RSP, (QWORD)pCpu->StackBase );

    // RIP
    // this will hold the address of our vmexit handler

    // CS, SS, DS, ES, FS, GS and TR selectors
    // CS, selector must be != 0x0
    VmxWrite( VMCS_HOST_CS_SELECTOR, VM_HOST_CODE_SEGMENT_SELECTOR );

    // SS
    VmxWrite( VMCS_HOST_SS_SELECTOR, VM_HOST_DATA_SEGMENT_SELECTOR );

    // DS
    VmxWrite( VMCS_HOST_DS_SELECTOR, 0 );

    // ES
    VmxWrite( VMCS_HOST_ES_SELECTOR, 0 );

    // FS
    VmxWrite( VMCS_HOST_FS_SELECTOR, VM_HOST_DATA_SEGMENT_SELECTOR );

    // GS
    VmxWrite( VMCS_HOST_GS_SELECTOR, VM_HOST_DATA_SEGMENT_SELECTOR );

    // TR, selector must be != 0x0
    VmxWrite( VMCS_HOST_TR_SELECTOR, pCpu->TrSelector );

    // FS, GS, TR, GDTR and IDTR base-address fields

    // FS, this should be done for each CPU
    VmxWrite( VMCS_HOST_FS_BASE, (QWORD)GetCurrentVcpu() );

    // GS, this should be done for each CPU
    VmxWrite( VMCS_HOST_GS_BASE, (QWORD)pCpu );

    // TR, this should probably be a legit value
    VmxWrite( VMCS_HOST_TR_BASE, (QWORD)pCpu->TssAddress );

    // GDTR
    // will need to save it in the yasm code
    VmxWrite( VMCS_HOST_GDTR_BASE, (QWORD)gGlobalData.Gdt->Base );

    // IDTR
    VmxWrite( VMCS_HOST_IDTR_BASE, (QWORD)gGlobalData.Idt->Base );

    // MSRs

    // IA32_SYSENTER_CS
    VmxWrite( VMCS_HOST_IA32_SYSENTER_CS, 0 );

    // IA32_SYSENTER_ESP
    VmxWrite( VMCS_HOST_IA32_SYSENTER_ESP, 0 );

    // IA32_SYSENTER_EIP
    VmxWrite( VMCS_HOST_IA32_SYSENTER_EIP, 0 );

    // IA32_PERF_GLOBAL_CTRL for processors that support
    // 1-setting of "load IA32_PERF_GLOBAL_CTRL"
    VmxWrite( VMCS_HOST_IA32_PERF_GLOBAL_CTRL_FULL, 0 );

    // IA32_PAT for processors that support
    // 1-setting of "load IA32_PAT"
    if (IsBooleanFlagOn(gGlobalData.VmxConfigurationData.VmxExitControls.AllowedOneSetting, EXIT_CONTROL_LOAD_IA32_PAT))
    {
        VmxWrite(VMCS_HOST_IA32_PAT_FULL, __readmsr(IA32_PAT));
    }

    // IA32_EFER for processors that support
    // 1-setting of "load IA32_EFER"
    if (IsBooleanFlagOn(gGlobalData.VmxConfigurationData.VmxExitControls.AllowedOneSetting, EXIT_CONTROL_LOAD_IA32_EFER))
    {
        VmxWrite(VMCS_HOST_IA32_EFER_FULL, __readmsr(IA32_EFER));
    }

    return STATUS_SUCCESS;
}

static
STATUS
VmcsInitializeVMXControls(
    void
    )
{
    STATUS status;
    DWORD tempValue;
    DWORD pinBasedControls;
    DWORD primaryProcBasedControls;
    DWORD secondaryProcBasedControls;

    // 24.6 VM-Execution Control Fields

#pragma region VM-Execution Control Fields

    // 24.6.1 Pin-Based VM-Execution Controls
    tempValue = VM_CONTROL_PIN_BASED_CTLS;
    LOG( "About to check Pin-Based Controls\n" );
    status = VmxWriteControlsAfterCheckingCapabilities( VMCS_CONTROL_PINBASED_CONTROLS, &(gGlobalData.VmxConfigurationData.PinBasedControls), &tempValue );
    ASSERT( SUCCEEDED( status ) );
    pinBasedControls = tempValue;

    // 24.6.2 Processor-Based VM-Execution Controls
    tempValue = VM_CONTROL_PRIMARY_PROC_BASED_CTLS;
    LOG( "About to check Primary Processor-Based Controls\n" );
    status = VmxWriteControlsAfterCheckingCapabilities( VMCS_CONTROL_PRIMARY_PROCBASED_CONTROLS, &(gGlobalData.VmxConfigurationData.PrimaryProcessorBasedControls), &tempValue );
    ASSERT( SUCCEEDED( status ) );
    primaryProcBasedControls = tempValue;

    tempValue = VM_CONTROL_SECONDARY_PROC_BASED_CTLS;
    status = VmxWriteControlsAfterCheckingCapabilities( VMCS_CONTROL_SECONDARY_PROCBASED_CONTROLS, &(gGlobalData.VmxConfigurationData.SecondaryProcessorBasedControls), &tempValue );
    ASSERT( SUCCEEDED( status ) );
    secondaryProcBasedControls = tempValue;

    // 24.6.3 Exception Bitmap

    // we don't want to treat any exceptions, we're lazy :)
    if( GetCurrentPcpu()->BspProcessor )
    {
        VmxWrite( VMCS_CONTROL_EXCEPTION_BITMAP, VM_CONTROL_EXCEPTION_BITMAP_BSP );
    }
    else
    {
        VmxWrite(VMCS_CONTROL_EXCEPTION_BITMAP,
                 gGlobalData.MiniHvInformation.RunningNested ? VM_CONTROL_EXCEPTION_BITMAP_AP_NESTED : VM_CONTROL_EXCEPTION_BITMAP_AP );
    }



    // 24.6.4 I/O Bitmap Addresses
    // used only if "use I/O bitmaps" is set
    if( IsBooleanFlagOn( primaryProcBasedControls, PROC_BASED_PRIMARY_USE_IO_BITMAPS ) )
    {
        // we need to set Bitmaps
        LOG( "We are using IO bitmaps :)\n" );
    }
    else
    {
        LOG( "We are not using IO Bitmaps :)\n" );
    }


    // 24.6.5 Time-Stamp Counter Offset
    // If "RDTSC exit" is clear and "use TSC offseting" is set
    // else unused
    if( !IsBooleanFlagOn( primaryProcBasedControls, PROC_BASED_PRIMARY_RDTSC_EXIT ) && IsBooleanFlagOn( primaryProcBasedControls, PROC_BASED_PRIMARY_TSC_OFFSETING ) )
    {
        // we need to set TSC offset
        NOT_REACHED;
    }
    else
    {
        LOG( "We are not using Time-Stamp Coutner Offset :)\n" );
    }

    // 24.6.6 Guest/Host Masks and Read Shadows for CR0 and CR4
    VmxWrite( VMCS_CONTROL_CR0_MASK, VM_CONTROL_CR0_MASK );

    VmxWrite( VMCS_CONTROL_CR0_READ_SHADOW, VM_CONTROL_CR0_SHADOW );

    VmxWrite( VMCS_CONTROL_CR4_MASK, VM_CONTROL_CR4_MASK );

    VmxWrite( VMCS_CONTROL_CR4_READ_SHADOW, VM_CONTROL_CR4_SHADOW );

    // 24.6.7 CR3-Target Controls

    // we want each MOV to CR3 to cause a VM Exit
    VmxWrite( VMCS_CONTROL_CR3_TARGET_COUNT, VM_CONTROL_CR3_TARGET_COUNT );

    if( 0 != VM_CONTROL_CR3_TARGET_COUNT )
    {
        NOT_REACHED;
        // should be set to proper values if we're here

        /*
        VmxWrite( VMCS_CONTROL_CR3_TARGET_0, 0 );
        VmxWrite( VMCS_CONTROL_CR3_TARGET_1, 0 );
        VmxWrite( VMCS_CONTROL_CR3_TARGET_2, 0 );
        VmxWrite( VMCS_CONTROL_CR3_TARGET_3, 0 );
        */
    }
    else
    {
        LOG( "We are not using CR3-Targets :)\n" );
    }


    // 24.6.8 Controls for APIC Virtualization
    // 5 processor-based VM-Execution controls that affect APIC accesses:
    //      1. "use TPR shadow"
    //      2. "virtualize APIC accesses"
    //      3. "virtualize x2APIC mode"
    //      4. "virtual-interrupt delivery"
    //      5. "APIC-register virtualization"
    if( IsBooleanFlagOn( secondaryProcBasedControls, PROC_BASED_SECONDARY_VIRTUALIZE_APIC_ACCESS ) )
    {
        // we need to set APIC-access address
        NOT_REACHED;
    }
    else
    {
        LOG( "We are not using virtualize APIC accesses :)\n" );
    }

    // Virtual-APIC address must be set
    VmxWrite( VMCS_CONTROL_VIRTUAL_APIC_ADDRESS_FULL, 0 );

    // only on processors which have the 1 setting
    if( IsBooleanFlagOn( primaryProcBasedControls, PROC_BASED_PRIMARY_USE_TPR_SHADOW ) )
    {
        VmxWrite( VMCS_CONTROL_TPR_THRESHOLD, 0 );
    }

    if( IsBooleanFlagOn( secondaryProcBasedControls, PROC_BASED_SECONDARY_VIRTUAL_INTERRUPT_DELIVERY ) )
    {
        // need to set EOI-exit bitmap
        NOT_REACHED;
    }
    else
    {
        LOG( "We don't have Virtual Interrupt Delivery :)\n" );
    }

    if( IsBooleanFlagOn( pinBasedControls, PIN_BASED_PROCESS_POSTED_INTS ) )
    {
        // need to set Posted-interrupt notification vector
        // need to set Posted-interrupt descriptor address
        NOT_REACHED;
    }
    else
    {
        LOG( "We don't have Process Posted Interrupts :)\n" );
    }

    // 24.6.9 MSR-Bitmap Address

    // if "use MSR bitmaps" is set
    if( IsBooleanFlagOn( primaryProcBasedControls, PROC_BASED_PRIMARY_USE_MSR_BITMAPS ) )
    {
        // need to set MSR bitmaps
        LOG( "We are using MSR bitmaps :)\n" );
    }
    else
    {
        LOG( "We are not using MSR bitmaps :)\n" );
    }

    // 24.6.10 Executive-VMCS Pointer

    // used in dual-monitor treatment of system-management interrupts (SMIs)
    VmxWrite( VMCS_CONTROL_EXECUTIVE_VMCS_POINTER_FULL, 0 );

    // 24.6.11 Extended-Page-Table Pointer

    if( IsBooleanFlagOn( secondaryProcBasedControls, PROC_BASED_SECONDARY_ENABLE_EPT ) )
    {
        /// EPT page table pointer
    }
    else
    {
        LOG( "We are not using EPT :)\n" );
    }

    // 24.6.12 Virtual-Processor Identifier

    // if "enable VPID" is set
    if( IsBooleanFlagOn( secondaryProcBasedControls, PROC_BASED_SECONDARY_ENABLE_VPID ) )
    {
        // TODO: set to a proper value
        NOT_REACHED;
    }
    else
    {
        LOG( "We are not using VPID :)\n" );
    }

    // 24.6.13 Controls for PAUSE-Loop Exiting

    // if "PAUSE-loop exiting" is set
    if( IsBooleanFlagOn( secondaryProcBasedControls, PROC_BASED_SECONDARY_PAUSE_LOOP_EXIT ) )
    {
        // TODO: set to a proper value
        NOT_REACHED;
    }
    else
    {
        LOG( "We are not using Pause Loop Exit :)\n" );
    }

    // 24.6.14 VM-Function Controls

    // if "enable VM functions" is set
    if( IsBooleanFlagOn( secondaryProcBasedControls, PROC_BASED_SECONDARY_ENABLE_VM_FUNCS ) )
    {
        // TODO: set to a proper value
        NOT_REACHED;
    }
    else
    {
        LOG( "We are not using VM Functions :)\n" );
    }

    // 24.6.15 VMCS Shadowing Bitmap Addresses

    // if "VMCS shadowing" is set
    if( IsBooleanFlagOn( secondaryProcBasedControls, PROC_BASED_SECONDARY_VMCS_SHADOWING ) )
    {
        // TODO: set to a proper value
        NOT_REACHED;
    }
    else
    {
        LOG( "We are not using VMCS Shadowing :)\n" );
    }

    // 24.6.16 Controls for Virtualization Exceptions

    // if "EPT-violation #VE" is set
    if( IsBooleanFlagOn( secondaryProcBasedControls, PROC_BASED_SECONDARY_EPT_VIOLATION_INTERRUPT ) )
    {
        // TODO: set to a proper value
        NOT_REACHED;
    }
    else
    {
        LOG( "We are not using EPT-violation #VE :)\n" );
    }

    // 24.6.19 XSS-Exiting Bitmap

    // if "enable XSAVES/XRSTORS" is set
    if( IsBooleanFlagOn( secondaryProcBasedControls, PROC_BASED_SECONDARY_ENABLE_XSAVES_XSTORS ) )
    {
        VmxWrite(VMCS_CONTROL_XSS_EXISTING_BITMAP_FULL, 0);
    }
    else
    {
        LOG( "We are not using XSAVES/XRSTORS :)\n" );
    }

#pragma endregion

    // 24.7 VM-Exit Control Fields

#pragma region VM-Exit Control Fields

    // 24.7.1 VM-Exit Controls
    tempValue = VM_CONTROL_EXIT_CTLS |
        (IsBooleanFlagOn(gGlobalData.VmxConfigurationData.PinBasedControls.ChosenValue, PIN_BASED_ACTIVATE_PREEMPT_TIMER) ? EXIT_CONTROL_SAVE_VMX_PREEMPT_TIMER_VALUE : 0);
    LOG( "About to check VM-Exit Controls\n" );
    status = VmxWriteControlsAfterCheckingCapabilities( VMCS_CONTROL_VM_EXIT_CONTROLS, &(gGlobalData.VmxConfigurationData.VmxExitControls), &tempValue );
    ASSERT( SUCCEEDED( status ) );

    // 24.7.2 VM-Exit Controls for MSRs
    VmxWrite( VMCS_CONTROL_VM_EXIT_MSR_STORE_COUNT, 0 );

    VmxWrite( VMCS_CONTROL_MSR_STORE_EXIT_ADDRESS_FULL, 0 );

    VmxWrite( VMCS_CONTROL_VM_EXIT_MSR_LOAD_COUNT, 0 );

    VmxWrite( VMCS_CONTROL_MSR_LOAD_EXIT_ADDRESS_FULL, 0 );
#pragma endregion

    // 24.8 VM-Entry Control Fields

#pragma region VM-Entry Control Fields

    // 24.8.1 VM-Entry Controls
    tempValue = VM_CONTROL_ENTRY_CTLS;
    LOG( "About to check VM-Entry Controls\n" );
    status = VmxWriteControlsAfterCheckingCapabilities( VMCS_CONTROL_VM_ENTRY_CONTROLS, &(gGlobalData.VmxConfigurationData.VmxEntryControls), &tempValue );
    ASSERT( SUCCEEDED( status ) );

    // 24.8.2 VM-Entry Controls for MSRs
    VmxWrite( VMCS_CONTROL_VM_ENTRY_MSR_LOAD_COUNT, 0 );

    VmxWrite( VMCS_CONTROL_MSR_LOAD_ENTRY_ADDRESS_FULL, 0 );

    // 24.8.3 VM-Entry Controls for Event Injection
    VmxWrite( VMCS_CONTROL_VM_ENTRY_INT_INFO_FIELD, 0 );

#pragma endregion

    return STATUS_SUCCESS;
}
#pragma warning(pop)
