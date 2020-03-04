#include "vmx_operations.h"
#include "vmcs.h"
#include "cpumu.h"
#include "log.h"
#include "display.h"
#include "apic.h"
#include "dmp_vmcs.h"
#include "paging_tables.h"
#include "native/memory.h"
#include "hv_heap.h"
#include "data.h"
#include "pci.h"

#pragma warning(push)

// warning C28039: The type of actual parameter '24576|2048|((0<<1))|0' should exactly match the type 'VMCS_FIELD':
#pragma warning(disable: 28039)

//******************************************************************************
// Function:      VmxRetrieveCapabilities
// Description: Retrieves processor capabilities.
// Returns:       STATUS
// Parameter:     OUT VMX_CONFIGURATION_DATA * VmxCapabilities
//******************************************************************************
static
STATUS
VmxRetrieveCapabilities(
    OUT     VMX_CONFIGURATION_DATA*        VmxCapabilities
    );

STATUS
VmxConfigureGlobalStructures(
    OUT     VMX_CONFIGURATION_DATA*         VmxConfiguration,
    OUT     VMX_SETTINGS*                   VmxSettings
)
{
    STATUS status;

    status = STATUS_SUCCESS;

    status = VmxRetrieveCapabilities(VmxConfiguration);
    if (!SUCCEEDED(status))
    {
        LOGL("VmxRetrieveCapabilities failed with status: 0x%x\n", status);
        return status;
    }

    return status;
}

STATUS
VmxCheckAndSetupCpu(
    void
    )
{
    STATUS status;
    QWORD cr0;
    QWORD cr4;
    QWORD featureControl;

    status = STATUS_SUCCESS;

    cr0 = __readcr0();

    LOGP( "Cr0 value: 0x%X\n", cr0 );

    cr4 = __readcr4();

    LOGP( "Cr4 value: 0x%X\n", cr4 );

    // this should have been set at CPU initialization
    ASSERT( cr4 & CR4_VMXE );

    // We need to set the lock bit in IA32_FEATURE_CONTROL
    featureControl = __readmsr( IA32_FEATURE_CONTROL );

    LOGP( "Feature control: 0x%X\n", featureControl );

    // we make sure the lock bit is set
    if( IsBooleanFlagOn(featureControl, IA32_FEATURE_LOCKED ))
    {
        ASSERT(IsBooleanFlagOn(featureControl, IA32_FEATURE_VMX_OUTSIDE_SMX));
    }
    else
    {
        featureControl = featureControl | (IA32_FEATURE_VMX_OUTSIDE_SMX | IA32_FEATURE_LOCKED);
        __writemsr(IA32_FEATURE_CONTROL, featureControl);
    }

    return status;
}

STATUS
VmxStartVmxOn(
    void
    )
{
    STATUS status;
    DWORD vmxResult;
    PCPU* pCurrentCpu;
    PVOID vmxOnRegionPhysicalAddress;

    status = STATUS_SUCCESS;


    pCurrentCpu = GetCurrentPcpu();

    *( ( DWORD * )( pCurrentCpu->VmxOnRegion ) ) = gGlobalData.VmxConfigurationData.VmcsRevisionIdentifier;
    vmxOnRegionPhysicalAddress = ( PVOID ) VA2PA( pCurrentCpu->VmxOnRegion );

    LOGP( "vmxOnRegion PA: 0x%X\n", vmxOnRegionPhysicalAddress );

    vmxResult = __vmx_on( &vmxOnRegionPhysicalAddress );

    LOGP( "__vmx_on result: %d\n", vmxResult );
    ASSERT( 0 == vmxResult );

    printf( "We executed VMXON on CPU %d\n", pCurrentCpu->ApicID );

    return status;
}


STATUS
VmxSetupVmcsStructures(
    IN      VMX_CONFIGURATION_DATA*      VmxCapabilities
    )
{
    STATUS status;
    PCPU* pCurrentCpu;
    VCPU* pVcpu;
    PVOID physicalVmcsRegionAddress;
    DWORD vmxResult;

    if( NULL == VmxCapabilities )
    {
        return STATUS_INVALID_PARAMETER1;
    }

    status = STATUS_SUCCESS;
    pCurrentCpu = GetCurrentPcpu();
    pVcpu = GetCurrentVcpu();

    // maybe we should assert if we're in VMX operation
    // don't know exactly how to do that

    // Step 1. Write VMCS Revision ID
    *( ( DWORD* ) pVcpu->VmcsRegion ) = VmxCapabilities->VmcsRevisionIdentifier;
    physicalVmcsRegionAddress = ( PVOID ) VA2PA( pVcpu->VmcsRegion );

    // (OPTIONAL) Step 2. Unmap VMCS region
    status = UnmapMemory( pVcpu->VmcsRegion, gGlobalData.VmxConfigurationData.VmcsRegionSize );
    ASSERT( SUCCEEDED( status ) );

    // Step 3. VMCLEAR
    vmxResult = __vmx_vmclear( &physicalVmcsRegionAddress );
    ASSERT( 0 == vmxResult );

    // Step 4. VMPTRLD
    vmxResult = __vmx_vmptrld( &physicalVmcsRegionAddress );
    ASSERT( 0 == vmxResult );

    // Step 5. oo x VMWRITE :)
    // here we will perform a lot of VMWRITE's
    status = VmcsInitializeRegion();

    LOG( "VmcsInitializeRegion status: 0x%X\n", status );
    ASSERT( SUCCEEDED( status ) );

    return status;
}

STATUS
VmxStartGuest(
    void
    )
{
    STATUS status;
    DWORD vmxResult;
    QWORD value;
    COMPLETE_PROCESSOR_STATE procState;

    status = STATUS_SUCCESS;
    vmxResult = 0;
    value = 0;
    memzero(&procState, sizeof(COMPLETE_PROCESSOR_STATE));

    GetCurrentVcpu()->EnteredVMX = TRUE;

    if( GetCurrentPcpu()->BspProcessor )
    {
        procState.RegisterArea.RegisterValues[RegisterRdx] = gGlobalData.VmxCurrentSettings.GuestPreloaderStartDiskDrive;
    }

    vmxResult = __vm_preLaunch(&procState);

    LOGP( "__vm_preLaunch result: %d\n", vmxResult );

    if( 1 == vmxResult )
    {
        // it means we have information

        ASSERT_INFO(FALSE, "We cannot continue because LAUNCH failed\n");
    }

    ASSERT( 0 == vmxResult );

    return status;
}

void
VmxResumeGuest(
    void
)
{
    VMX_RESULT vmxResult;
    STATUS status;
    VCPU* guestVcpu = GetCurrentVcpu();

    status = STATUS_SUCCESS;

    AssertHvIsStillFunctional();

    // we need an interlocked operation because we could be treating a normal
    // interrupt (non-NMI) and even though IF = 0 a NMI can come
    ASSERT(2 > _InterlockedCompareExchange(&(guestVcpu->PendingEventForInjection),0,1) );

    vmxResult = __vm_preResume( guestVcpu->ProcessorState );
    ASSERT( 0 == vmxResult );

    NOT_REACHED;
}

STATUS
VmxSetActivityState(
    IN  ACTIVITY_STATE      ActivityState
    )
{
    ASSERT(ActivityStateActive <= ActivityState && ActivityState <= ActivityStateWaitForSIPI);

    VmxWrite(VMCS_GUEST_ACTIVITY_STATE, ActivityState);

    return STATUS_SUCCESS;
}

STATUS
VmxRetrieveCapabilities(
    OUT      VMX_CONFIGURATION_DATA*     VmxCapabilities
)
{
    STATUS status;
    QWORD msrValue;

    if( NULL == VmxCapabilities )
    {
        return STATUS_INVALID_PARAMETER1;
    }

    status = STATUS_SUCCESS;

    msrValue = __readmsr( IA32_VMX_BASIC_MSR );

    // Read basic information
    VmxCapabilities->VmcsRevisionIdentifier = msrValue & 0x7FFFFFFFUL;
    VmxCapabilities->VmcsRegionSize = QWORD_HIGH( msrValue ) & 0x1FFF;

    VmxCapabilities->MemoryTypeSupported = ( msrValue >> 50 ) & 0xF;
    VmxCapabilities->InsOutExitInformation = IsBooleanFlagOn( msrValue,( ( QWORD ) 1 << 54 ) );

    VmxCapabilities->VmxTrueControlsSupported = IsBooleanFlagOn( msrValue, ( ( QWORD ) 1 << 55 ) );

    LOG( "VmcsRevisionIdentifier: 0x%X\n", VmxCapabilities->VmcsRevisionIdentifier );
    LOG( "VmcsRegionSize: %d bytes\n", VmxCapabilities->VmcsRegionSize );
    LOG( "VmxMemoryTypeSupported: %d\n", VmxCapabilities->MemoryTypeSupported );
    LOG( "InsOutExitInformation: %d\n", VmxCapabilities->InsOutExitInformation );
    LOG( "VmxTrueControlsSupported: %d\n", VmxCapabilities->VmxTrueControlsSupported );

    // PINBASED Controls
    LOG( "\n\nPinbased Controls:\n" );
    status = VmxCapabilityRetrieveInformation( &( VmxCapabilities->PinBasedControls ), VMX_CAPABILITIES_PINBASED, VmxCapabilities->VmxTrueControlsSupported );

    ASSERT( SUCCEEDED( status ) );

    // Primary PROCBASED Controls
    LOG( "\n\nPrimary Procbased Controls:\n" );
    status = VmxCapabilityRetrieveInformation( &( VmxCapabilities->PrimaryProcessorBasedControls ), VMX_CAPABILITIES_PROCBASED, VmxCapabilities->VmxTrueControlsSupported );

    ASSERT( SUCCEEDED( status ) );

    // Secondary PROCBASED Controls
    ASSERT_INFO(IsBooleanFlagOn(VmxCapabilities->PrimaryProcessorBasedControls.AllowedOneSetting, PROC_BASED_PRIMARY_ACTIVATE_SECONDARY_CTLS), "We cannot function without EPT or UG\n");


    // IA32_VMX_PROCBASED_CTLS2 MSR exists only if bit 63 of IA32_VMX_PROCBASED_CTLS MSR is 1
    LOG( "\n\nSecondary Procbased Controls:\n" );
    status = VmxCapabilityRetrieveInformation( &( VmxCapabilities->SecondaryProcessorBasedControls ), VMX_CAPABILITIES_SEC_PROCBASED, VmxCapabilities->VmxTrueControlsSupported );
    ASSERT( SUCCEEDED( status ) );

    ASSERT_INFO(IsBooleanFlagOn(VmxCapabilities->SecondaryProcessorBasedControls.AllowedOneSetting, ( PROC_BASED_SECONDARY_ENABLE_EPT | PROC_BASED_SECONDARY_UNRESTRICTED_GUEST ) ), "We cannot function without EPT or UG\n");

    status = VmxCapabilityCheckEptSupport( &( VmxCapabilities->EptSupport ) );
    ASSERT( SUCCEEDED( status ) );

    status = VmxCapabilityRetrieveAditionalOptions(&(VmxCapabilities->MiscOptions));
    ASSERT(SUCCEEDED(status));

    LOGL("Misc Options: 0x%X\n", *((QWORD*)&VmxCapabilities->MiscOptions));


    // EXIT Controls
    LOG( "\n\nExit Controls:\n" );
    status = VmxCapabilityRetrieveInformation( &( VmxCapabilities->VmxExitControls ), VMX_CAPABILITIES_EXIT, VmxCapabilities->VmxTrueControlsSupported );

    ASSERT( SUCCEEDED( status ) );

    // ENTRY Controls
    LOG( "\n\nEntry Controls:\n" );
    status = VmxCapabilityRetrieveInformation( &( VmxCapabilities->VmxEntryControls ), VMX_CAPABILITIES_ENTRY, VmxCapabilities->VmxTrueControlsSupported );

    ASSERT( SUCCEEDED( status ) );

    // Retrieve CR0 information
    LOG("\n\nCR0 Fixed Values:\n");
    status = VmxCapabilityRetrieveInformation( &( VmxCapabilities->Cr0Values ), VMX_CAPABILITIES_CR0, VmxCapabilities->VmxTrueControlsSupported );

    ASSERT( SUCCEEDED( status ) );

    // Retrieve CR4 information
    LOG("\n\nCR4 Fixed Values:\n");
    status = VmxCapabilityRetrieveInformation( &( VmxCapabilities->Cr4Values ), VMX_CAPABILITIES_CR4, VmxCapabilities->VmxTrueControlsSupported );

    ASSERT( SUCCEEDED( status ) );

    return status;
}
#pragma warning(pop)
