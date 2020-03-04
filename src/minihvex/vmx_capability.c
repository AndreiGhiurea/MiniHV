#include "vmx_capability.h"
#include "native/memory.h"
#include "log.h"
#include "vmcs.h"
#include "data.h"

#pragma warning(push)

// warning C28039: The type of actual parameter '24576|2048|((0<<1))|0' should exactly match the type 'VMCS_FIELD':
#pragma warning(disable: 28039)
static
STATUS
_VmxCapabilityCheckControls(
    IN      VMX_CAPABILITY_ARRAY*   Capabilities,
    INOUT   DWORD*                  ControlValues
    );

static
STATUS
_VmxCapabilityQueryGeneralControls(
    OUT     VMX_CAPABILITY_ARRAY*   Capabilities,
    IN      DWORD                   ControlRegisterIndex,
    IN      BOOLEAN                 TrueControls
    )
{
    STATUS status;

    DWORD msrBasicAddress;
    DWORD msrTrueAddress;

    QWORD msrBasicValue;
    QWORD msrTrueValue;

    BOOLEAN secProcBasedControls;

    // calculate MSR address
    secProcBasedControls = VMX_CAPABILITIES_SEC_PROCBASED == ControlRegisterIndex;
    if( !secProcBasedControls )
    {
        msrBasicAddress = IA32_VMX_PINBASED_CTLS + ControlRegisterIndex;
    }
    else
    {
        msrBasicAddress = IA32_VMX_PROCBASED_CTLS2;
    }

    msrTrueAddress = msrBasicAddress + VMX_TRUE_REGULAR_DIFFERENCE;

    msrBasicValue = __readmsr( msrBasicAddress );

    if( ( TrueControls ) && ( !secProcBasedControls ) )
    {
        // 3.c
        msrTrueValue = __readmsr( msrTrueAddress );

        LOG( "msrTrueValue: 0x%X\n", msrTrueValue );

        Capabilities->AllowedZeroSetting = ~( ( DWORD ) QWORD_LOW( msrTrueValue ) );
        Capabilities->AllowedOneSetting = QWORD_HIGH( msrTrueValue );
        Capabilities->FlexibleValue = Capabilities->AllowedZeroSetting & Capabilities->AllowedOneSetting;

        LOG( "Capabilities->AllowedZeroSetting: 0x%x\n", Capabilities->AllowedZeroSetting );
        LOG( "Capabilities->AllowedOneSetting: 0x%x\n", Capabilities->AllowedOneSetting );

        Capabilities->DefaultZeroClass = Capabilities->FlexibleValue & ~( ( DWORD ) QWORD_LOW( msrBasicValue ) );

        LOG( "Capabilities->FlexibleValue: 0x%x\n", Capabilities->FlexibleValue );
        LOG( "Capabilities->Default Zero Class: 0x%x\n", Capabilities->DefaultZeroClass );
    }
    else
    {
        // 3.b
        Capabilities->AllowedZeroSetting = ~( ( DWORD ) QWORD_LOW( msrBasicValue ) );
        Capabilities->AllowedOneSetting = QWORD_HIGH( msrBasicValue );
        Capabilities->FlexibleValue = Capabilities->AllowedZeroSetting & Capabilities->AllowedOneSetting;

        LOG( "Capabilities->AllowedZeroSetting: 0x%x\n", Capabilities->AllowedZeroSetting );
        LOG( "Capabilities->AllowedOneSetting: 0x%x\n", Capabilities->AllowedOneSetting );

        Capabilities->DefaultZeroClass = ~Capabilities->FlexibleValue & Capabilities->AllowedZeroSetting;

        LOG( "Capabilities->FlexibleValue: 0x%x\n", Capabilities->FlexibleValue );
        LOG( "Capabilities->Default Zero Class: 0x%x\n", Capabilities->DefaultZeroClass );
    }




    status = STATUS_SUCCESS;

    return status;
}

static
STATUS
_VmxCapabilityQueryControlRegister(
    OUT     VMX_CAPABILITY_ARRAY*   Capabilities,
    IN      DWORD                   ControlRegisterIndex
    )
{
    DWORD fixed0MsrRegister;
    DWORD fixed1MsrRegister;

    QWORD fixed0Values;
    QWORD fixed1Values;
    QWORD flexibleValues;

    fixed0MsrRegister = IA32_VMX_CRO_FIXED0 + ControlRegisterIndex;
    fixed1MsrRegister = fixed0MsrRegister + 1;

    fixed0Values = __readmsr( fixed0MsrRegister );
    fixed1Values = __readmsr( fixed1MsrRegister );

    LOG( "fixed0Values for CR%d: 0x%X\n", ( 0 == ControlRegisterIndex ) ? 0 : 4, fixed0Values );
    LOG( "fixed1Values for CR%d: 0x%X\n", ( 0 == ControlRegisterIndex ) ? 0 : 4, fixed1Values );

    flexibleValues = fixed0Values ^ fixed1Values;

    Capabilities->AllowedOneSetting = QWORD_LOW( fixed1Values );
    Capabilities->AllowedZeroSetting = ~QWORD_LOW(fixed0Values);

    if( IA32_VMX_CRO_FIXED0 == fixed0MsrRegister )
    {
        // it means it's CR0, => we must check for unrestricted guest support and add
        // AllowedZeroSetting to CR0.PG and CR0.PE
        if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.SecondaryProcessorBasedControls.AllowedOneSetting, PROC_BASED_SECONDARY_UNRESTRICTED_GUEST ) )
        {
            LOG("UG support\n");
            Capabilities->AllowedZeroSetting = Capabilities->AllowedZeroSetting | (CR0_PG | CR0_PE);
        }
    }

    Capabilities->FlexibleValue = ( DWORD ) ( Capabilities->AllowedOneSetting & Capabilities->AllowedZeroSetting );

    LOG( "Capabilities->AllowedOneSetting: 0x%x\n", Capabilities->AllowedOneSetting );
    LOG( "Capabilities->AllowedZeroSetting: 0x%x\n", Capabilities->AllowedZeroSetting );
    LOG( "Capabilities->FlexibleValue: 0x%x\n", Capabilities->FlexibleValue );

    return STATUS_SUCCESS;
}

STATUS
VmxCapabilityRetrieveInformation(
    OUT     VMX_CAPABILITY_ARRAY*   Capabilities,
    IN      DWORD                   CapabilityIndex,
    IN      BOOLEAN                 TrueControls
    )
{
    STATUS status;

    if( NULL == Capabilities )
    {
        return STATUS_INVALID_PARAMETER1;
    }

    if( !( ( VMX_CAPABILITIES_PINBASED <= CapabilityIndex ) || ( VMX_CAPABILITIES_CR4 >= CapabilityIndex ) ) )
    {
        return STATUS_INVALID_PARAMETER2;
    }

    status = STATUS_SUCCESS;

    if( CapabilityIndex >= VMX_CAPABILITIES_CR0 )
    {
        // we need to query the capabilities of CR0 or CR4
        return _VmxCapabilityQueryControlRegister( Capabilities, ( CapabilityIndex - VMX_CAPABILITIES_CR0 ) * 2 );
    }
    else
    {
        // CapabilityIndex < VMX_CAPABILITIES_CR0
        return _VmxCapabilityQueryGeneralControls( Capabilities, ( CapabilityIndex - VMX_CAPABILITIES_PINBASED ), TrueControls );
    }
}

STATUS
VmxWriteControlsAfterCheckingCapabilities(
    IN      DWORD                   VmcsField,
    INOUT   VMX_CAPABILITY_ARRAY*   Capabilities,
    INOUT   DWORD*                  ControlValues
)
{
    if( NULL == Capabilities )
    {
        return STATUS_INVALID_PARAMETER2;
    }

    if( NULL == ControlValues )
    {
        return STATUS_INVALID_PARAMETER3;
    }

    _VmxCapabilityCheckControls( Capabilities, ControlValues );

    VmxWrite( VmcsField, *ControlValues );

    Capabilities->ChosenValue = *ControlValues;

    return STATUS_SUCCESS;
}

STATUS
VmxCapabilityCheckEptSupport(
    OUT     EPT_SUPPORT*            EptSupport
    )
{
    STATUS status;
    QWORD msrValue;

    if( NULL == EptSupport )
    {
        return STATUS_INVALID_PARAMETER1;
    }

    status = STATUS_SUCCESS;

    msrValue = __readmsr( IA32_VMX_EPT_VPID_CAP );

    EptSupport->SupportAvailable = 1;
    EptSupport->ExecuteOnly = IsBooleanFlagOn( msrValue, EPT_EXECUTE_ONLY_SUPPORT );
    EptSupport->PageWalkLength4 = IsBooleanFlagOn( msrValue, EPT_PAGE_WALK_4_SUPPORT );
    if (IsBooleanFlagOn( msrValue, EPT_WB_MEMORY_SUPPORT ))
    {
        EptSupport->MemoryType = MEMORY_TYPE_WRITEBACK;
    }
    else
    {
        ASSERT(IsBooleanFlagOn(msrValue, EPT_UC_MEMORY_SUPPORT));

        EptSupport->MemoryType = MEMORY_TYPE_UNCACHEABLE;
    }

    LOG( "EptSupport->ExecuteOnly: 0x%x\n", EptSupport->ExecuteOnly );
    LOG( "EptSupport->PageWalkLength4: 0x%x\n", EptSupport->PageWalkLength4 );
    LOG( "EptSupport->MemoryType: 0x%x\n", EptSupport->MemoryType );
    LOG( "IA32_VMX_EPT_VPID_CAP: 0x%X\n", msrValue);

    return status;
}

STATUS
VmxCapabilityRetrieveAditionalOptions(
    OUT     VMX_MISC_OPTIONS*       MiscOptions
    )
{
    QWORD msrValue;

    if( NULL == MiscOptions )
    {
        return STATUS_INVALID_PARAMETER1;
    }

    msrValue = __readmsr( IA32_VMX_MISC );
    memcpy( MiscOptions, &msrValue, sizeof( VMX_MISC_OPTIONS ) );

    ASSERT(MiscOptions->Ia32eAutoStore);

    LOG("Support for HLT state: %d\n", MiscOptions->HltActivityState);
    LOG("Support for shutdown state: %d\n", MiscOptions->ShutdownActivityState);
    LOG("Support for wait-for-SIPI state: %d\n", MiscOptions->WaitForSipiActivityState);

    ASSERT( MiscOptions->HltActivityState | MiscOptions->ShutdownActivityState | MiscOptions->WaitForSipiActivityState);

    return STATUS_SUCCESS;
}

STATUS
_VmxCapabilityCheckControls(
    IN      VMX_CAPABILITY_ARRAY*   Capabilities,
    INOUT   DWORD*                  ControlValues
    )
{
    STATUS status;
    DWORD value;

    if( NULL == Capabilities )
    {
        return STATUS_INVALID_PARAMETER1;
    }

    if( NULL == ControlValues )
    {
        return STATUS_INVALID_PARAMETER2;
    }

    status = STATUS_SUCCESS;
    value = *ControlValues;

    // set all the 1s
    value = ( value | ( ( ~Capabilities->FlexibleValue ) & Capabilities->AllowedOneSetting ) );

    // remove the 0s
    value = ( value & ( ( Capabilities->FlexibleValue ) ^ (~Capabilities->AllowedZeroSetting ) ) );

    //LOG( "Initial control values: 0x%X\n", *ControlValues );
    //LOG( "Final control values: 0x%X\n", value );
    *ControlValues = value;

    return status;
}
#pragma warning(pop)
