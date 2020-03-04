#include "dmp_vmcs.h"
#include "log.h"
#include "apic.h"
#include "vmx_operations.h"
#include "dmp_memory.h"
#include "segment.h"
#include "idt_handlers.h"
#include "vmguest.h"
#include "dmp_cpu.h"
#include "native/memory.h"
#include "data.h"
#include "dmp_common.h"

#pragma warning(push)

// warning C28039: The type of actual parameter '24576|2048|((0<<1))|0' should exactly match the type 'VMCS_FIELD':
#pragma warning(disable: 28039)

static
void
DumpCurrentVmcsGuestState(
    void
    );

static
void
DumpCurrentVmcsHostState(
    void
    );

static
void
DumpCurrentVmcsControlFields(
    void
    );

static
void
DumpCurrentVmcsExecutionControlFields(
    void
    );

static
void
DumpCurrentVmcsExitControlFields(
    void
    );

static
void
DumpCurrentVmcsEntryControlFields(
    void
    );

void
DumpCurrentVmcs(
    IN_OPT      REGISTER_AREA*        ProcessorState
    )
{
    INTR_STATE oldState = DumpTakeLock();

    // if we don't do 2 different LOGS we will get
    // [CPU:xx] before the newlines
    LOG( "\n\n\n" );
    LOGP( "VMCS dump\n" );

    // Processor State
    if( NULL != ProcessorState )
    {
        DumpRegisterArea( ProcessorState );
    }


    // Guest-State area

    DumpCurrentVmcsGuestState();

    // Host-State Area

    DumpCurrentVmcsHostState();

    // Control Fields

    DumpCurrentVmcsControlFields();

    LOGP( "End VMCS dump\n\n\n" );

    DumpReleaseLock(oldState);
}

static
void
DumpCurrentVmcsGuestState(
    void
    )
{
    QWORD fieldValue;
    GDT guestGdt;
    IDT guestIdt;
    QWORD guestCr0;
    QWORD guestCr3;
    QWORD guestRIP;
    QWORD csBaseAddress;
    QWORD instrLength;
    PVOID baseValue;
    PVOID instructionVA;
    STATUS status;

    LOG( "-------------------------------------\n" );
    LOG( "Guest-State Area\n")
    LOG( "-------------------------------------\n" );

    LOG( "\nGuest Register State\n" );

    fieldValue = 0;
    memzero(&guestGdt, sizeof(GDT));
    memzero(&guestIdt, sizeof(IDT));
    guestCr0 = 0;
    guestCr3 = 0;
    guestRIP = 0;
    csBaseAddress = 0;
    instrLength = 0;
    instructionVA = 0;
    baseValue = NULL;
    status = STATUS_SUCCESS;

#pragma region Guest Register State
    // CR0
    fieldValue = VmxRead(VMCS_GUEST_CR0);
    LOG( "CR0: 0x%X\n", fieldValue );

    guestCr0 = fieldValue;

    // CR3
    fieldValue = VmxRead(VMCS_GUEST_CR3);
    LOG( "CR3: 0x%X\n", fieldValue );

    guestCr3 = fieldValue;

    // CR4
    fieldValue = VmxRead(VMCS_GUEST_CR4);
    LOG( "CR4: 0x%X\n", fieldValue );

    // DR7
    fieldValue = VmxRead(VMCS_GUEST_DR7);
    LOG( "DR7: 0x%X\n", fieldValue );

    // RSP
    fieldValue = VmxRead(VMCS_GUEST_RSP);
    LOG( "RSP: 0x%X\n", fieldValue );

    // RIP
    fieldValue = VmxRead(VMCS_GUEST_RIP);
    LOG( "RIP: 0x%X\n", fieldValue );

    guestRIP = fieldValue;

    fieldValue = VmxRead( VMCS_GUEST_CS_BASE);
    csBaseAddress = fieldValue;

    // we need to add base address of CS to guestRIP
    guestRIP = csBaseAddress + guestRIP;

    // Instruction Length
    fieldValue = VmxRead(VMCS_EXIT_INSTRUCTION_LENGTH);

    instrLength = fieldValue;

    status = GuestVAToHostVA( ( PVOID ) guestRIP, NULL, &instructionVA );
    if( SUCCEEDED( status ) )
    {
        LOG( "RIP[VA]: 0x%X\n", instructionVA );
        LOG( "Instruction[%d]: ", instrLength );
        DumpMemory( instructionVA, 0, 0x10, FALSE, TRUE );
    }
    else
    {
        LOG( "[ERROR] GuestVAToHostVA failed with status: 0x%X\n", status );
    }


    // RFLAGS
    fieldValue = VmxRead( VMCS_GUEST_RFLAGS);
    LOG( "RFLAGS: 0x%X\n", fieldValue );

    // CS
    LOG( "\nCS:\n" );
    // CS.Selector
    fieldValue = VmxRead( VMCS_GUEST_CS_SELECTOR);

    LOG( "Selector: 0x%X\n", fieldValue );

    // CS.Address
    LOG( "Base Address: 0x%X\n", csBaseAddress );

    // CS.Limit
    fieldValue = VmxRead( VMCS_GUEST_CS_LIMIT);

    LOG( "Limit: 0x%X\n", fieldValue );

    // CS.AccessRights
    fieldValue = VmxRead( VMCS_GUEST_CS_ACCESS_RIGHTS);

    LOG( "Access Rights: 0x%X\n", fieldValue );

    // SS
    LOG( "\nSS:\n" );
    // SS.Selector
    fieldValue = VmxRead( VMCS_GUEST_SS_SELECTOR);

    LOG( "Selector: 0x%X\n", fieldValue );

    // SS.Address
    fieldValue = VmxRead( VMCS_GUEST_SS_BASE);
    LOG( "Base Address: 0x%X\n", fieldValue );

    // SS.Limit
    fieldValue = VmxRead( VMCS_GUEST_SS_LIMIT);
    LOG( "Limit: 0x%X\n", fieldValue );

    // SS.AccessRights
    fieldValue = VmxRead( VMCS_GUEST_SS_ACCESS_RIGHTS);
    LOG( "Access Rights: 0x%X\n", fieldValue );



    // DS
    LOG( "\nDS:\n" );
    // DS.Selector
    fieldValue = VmxRead( VMCS_GUEST_DS_SELECTOR);
    LOG( "Selector: 0x%X\n", fieldValue );

    // DS.Address
    fieldValue = VmxRead( VMCS_GUEST_DS_BASE);
    LOG( "Base Address: 0x%X\n", fieldValue );

    // DS.Limit
    fieldValue = VmxRead( VMCS_GUEST_DS_LIMIT);
    LOG( "Limit: 0x%X\n", fieldValue );

    // DS.AccessRights
    fieldValue = VmxRead( VMCS_GUEST_DS_ACCESS_RIGHTS);
    LOG( "Access Rights: 0x%X\n", fieldValue );


    // ES
    LOG( "\nES:\n" );
    // ES.Selector
    fieldValue = VmxRead( VMCS_GUEST_ES_SELECTOR);
    LOG( "Selector: 0x%X\n", fieldValue );

    // ES.Address
    fieldValue = VmxRead( VMCS_GUEST_ES_BASE);
    LOG( "Base Address: 0x%X\n", fieldValue );

    // ES.Limit
    fieldValue = VmxRead( VMCS_GUEST_ES_LIMIT);
    LOG( "Limit: 0x%X\n", fieldValue );

    // ES.AccessRights
    fieldValue = VmxRead( VMCS_GUEST_ES_ACCESS_RIGHTS);
    LOG( "Access Rights: 0x%X\n", fieldValue );

    // FS
    LOG( "\nFS:\n" );
    // FS.Selector
    fieldValue = VmxRead( VMCS_GUEST_FS_SELECTOR);
    LOG( "Selector: 0x%X\n", fieldValue );

    // FS.Address
    fieldValue = VmxRead( VMCS_GUEST_FS_BASE);
    LOG( "Base Address: 0x%X\n", fieldValue );

    // FS.Limit
    fieldValue = VmxRead( VMCS_GUEST_FS_LIMIT);
    LOG( "Limit: 0x%X\n", fieldValue );

    // FS.AccessRights
    fieldValue = VmxRead( VMCS_GUEST_FS_ACCESS_RIGHTS);
    LOG( "Access Rights: 0x%X\n", fieldValue );

    // GS
    LOG( "\nGS:\n" );
    // GS.Selector
    fieldValue = VmxRead( VMCS_GUEST_GS_SELECTOR);
    LOG( "Selector: 0x%X\n", fieldValue );

    // GS.Address
    fieldValue = VmxRead( VMCS_GUEST_GS_BASE);
    LOG( "Base Address: 0x%X\n", fieldValue );

    // GS.Limit
    fieldValue = VmxRead( VMCS_GUEST_GS_LIMIT);
    LOG( "Limit: 0x%X\n", fieldValue );

    // GS.AccessRights
    fieldValue = VmxRead( VMCS_GUEST_GS_ACCESS_RIGHTS);
    LOG( "Access Rights: 0x%X\n", fieldValue );

    // LDTR
    LOG( "\nLDTR:\n" );
    // LDTR.Selector
    fieldValue = VmxRead( VMCS_GUEST_LDTR_SELECTOR);
    LOG( "Selector: 0x%X\n", fieldValue );

    // LDTR.Address
    fieldValue = VmxRead( VMCS_GUEST_LDTR_BASE);
    LOG( "Base Address: 0x%X\n", fieldValue );

    // LDTR.Limit
    fieldValue = VmxRead( VMCS_GUEST_LDTR_LIMIT);
    LOG( "Limit: 0x%X\n", fieldValue );

    // LDTR.AccessRights
    fieldValue = VmxRead( VMCS_GUEST_LDTR_ACCESS_RIGHTS);
    LOG( "Access Rights: 0x%X\n", fieldValue );

    // TR
    LOG( "\nTR:\n" );
    // TR.Selector
    fieldValue = VmxRead( VMCS_GUEST_TR_SELECTOR);
    LOG( "Selector: 0x%X\n", fieldValue );

    // TR.Address
    fieldValue = VmxRead( VMCS_GUEST_TR_BASE);
    LOG( "Base Address: 0x%X\n", fieldValue );

    // TR.Limit
    fieldValue = VmxRead( VMCS_GUEST_TR_LIMIT);
    LOG( "Limit: 0x%X\n", fieldValue );

    // TR.AccessRights
    fieldValue = VmxRead( VMCS_GUEST_TR_ACCESS_RIGHTS);
    LOG( "Access Rights: 0x%X\n", fieldValue );

    // GDTR
    LOG( "\nGDTR:\n" );

    // GDTR.Limit
    fieldValue = VmxRead(VMCS_GUEST_GDTR_LIMIT);

    guestGdt.Limit = (WORD)fieldValue;

    // GDTR.Address
    fieldValue = VmxRead( VMCS_GUEST_GDTR_BASE);
    LOG( "Base Address: 0x%X\n", fieldValue );

    status = GuestVAToHostVA((PVOID)fieldValue, NULL, &baseValue);
    ASSERT_INFO(SUCCEEDED(status), "Failed with status: 0x%x\n", status );

    guestGdt.Base = baseValue;

    LOG("Limit: 0x%x\n", guestGdt.Limit);

    LOG( "GDT entries: \n" );

    if( ( NULL != guestGdt.Base ) && ( 0 != guestGdt.Limit ) )
    {
        DumpMemory( ( PVOID ) guestGdt.Base, 0, guestGdt.Limit + 1, FALSE, TRUE );
    }

    // IDTR
    LOG( "\nIDTR:\n" );
    // IDTR.Address
    fieldValue = VmxRead( VMCS_GUEST_IDTR_BASE);


    LOG( "Base Address: 0x%X\n", fieldValue );

    if (0 != fieldValue)
    {
        status = GuestVAToHostVA((PVOID)fieldValue, NULL, &baseValue);
        ASSERT_INFO(SUCCEEDED(status), "Failed with status: 0x%x\n", status);
    }
    else
    {
        baseValue = NULL;
    }

    guestIdt.Base = baseValue;


    // IDTR.Limit
    fieldValue = VmxRead( VMCS_GUEST_IDTR_LIMIT);
    LOG( "Limit: 0x%X\n", fieldValue );

    if( IsBooleanFlagOn( guestCr0, CR0_PE ) )
    {
        guestIdt.Limit = ( WORD ) fieldValue;
    }
    else
    {
        guestIdt.Limit = ( WORD ) IVT_LIMIT;
    }


    LOG( "IDT entries: \n" );
    if( ( NULL != guestIdt.Base ) && ( 0 != guestIdt.Limit ) )
    {
        DumpMemory( ( PVOID ) guestIdt.Base, 0, guestIdt.Limit + 1, FALSE, TRUE );
    }


    LOG( "\nMSRs:\n" );
    // IA32_DEBUGCTL
    fieldValue = VmxRead( VMCS_GUEST_IA32_DEBUGCTL_FULL);
    LOG( "IA32_DEBUGCTL: 0x%X\n", fieldValue );

    // IA32_SYSENTER_CS
    fieldValue = VmxRead( VMCS_GUEST_IA32_SYSENTER_CS);
    LOG( "IA32_SYSENTER_CS: 0x%X\n", fieldValue );

    // IA32_SYSENTER_ESP
    fieldValue = VmxRead( VMCS_GUEST_IA32_SYSENTER_ESP);
    LOG( "IA32_SYSENTER_ESP: 0x%X\n", fieldValue );

    // IA32_SYSENTER_EIP
    fieldValue = VmxRead( VMCS_GUEST_IA32_SYSENTER_EIP);
    LOG( "IA32_SYSENTER_EIP: 0x%X\n", fieldValue );

    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.VmxEntryControls.AllowedOneSetting, ENTRY_CONTROL_LOAD_IA32_PERF_GLOBAL_CTRL ) )
    {
        // IA32_PERF_GLOBAL_CONTROL
        fieldValue = VmxRead( VMCS_GUEST_IA32_PERF_GLOBAL_CTRL_FULL);
        LOG( "IA32_PERF_GLOBAL_CONTROL: 0x%X\n", fieldValue );
    }
    else
    {
        LOG( "We don't have load IA32_PERF_GLOBAL_CTRL :(\n" );
    }

    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.VmxEntryControls.AllowedOneSetting, ENTRY_CONTROL_LOAD_IA32_PAT ) ||
        IsBooleanFlagOn( gGlobalData.VmxConfigurationData.VmxExitControls.AllowedOneSetting, EXIT_CONTROL_SAVE_IA32_PAT ) )
    {
        // IA32_PAT
        fieldValue = VmxRead( VMCS_GUEST_IA32_PAT_FULL);
        LOG( "IA32_PAT: 0x%X\n", fieldValue );
    }
    else
    {
        LOG( "We don't have load/save IA32_PAT :(\n")
    }

    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.VmxEntryControls.AllowedOneSetting, ENTRY_CONTROL_LOAD_IA32_EFER ) ||
        IsBooleanFlagOn( gGlobalData.VmxConfigurationData.VmxExitControls.AllowedOneSetting, EXIT_CONTROL_SAVE_IA32_EFER ) )
    {
        // IA32_EFER
        fieldValue = VmxRead( VMCS_GUEST_IA32_EFER_FULL);
        LOG( "IA32_EFER: 0x%X\n", fieldValue );
    }
    else
    {
        LOG( "We don't have load/save IA32_EFER :(\n")
    }

    // SMBASE
    fieldValue = VmxRead( VMCS_GUEST_SMBASE);
    LOG( "SMBASE: 0x%X\n", fieldValue );

#pragma endregion

    LOG( "\nGuest Non-Register State\n" );

#pragma region Guest Non-Register State

    // Activity State
    fieldValue = VmxRead( VMCS_GUEST_ACTIVITY_STATE);
    LOG( "Activity State: 0x%X\n", fieldValue );

    // Interruptibillity State
    fieldValue = VmxRead( VMCS_GUEST_INT_STATE);
    LOG( "Interruptibillity State: 0x%X\n", fieldValue );

    // Pending Debug Exceptions
    fieldValue = VmxRead( VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS);
    LOG( "Pending Debug Exceptions: 0x%X\n", fieldValue );

    // VMCS Link Pointer
    fieldValue = VmxRead( VMCS_GUEST_VMCS_LINK_POINTER_FULL);
    LOG( "VMCS Link Pointer: 0x%X\n", fieldValue );

    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.PinBasedControls.AllowedOneSetting, PIN_BASED_ACTIVATE_PREEMPT_TIMER ) )
    {
        // VMX-preemption timer value
        fieldValue = VmxRead( VMCS_GUEST_VMX_PREEMPT_TIMER_VALUE);
        LOG( "VMX-preemption timer value: 0x%X\n", fieldValue );
    }
    else
    {
        LOG( "We don't have VMX-preemption timer :(\n" );
    }

    // PDPTEs
    LOG( "\nPDPTEs:\n" );
    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.SecondaryProcessorBasedControls.AllowedOneSetting, PROC_BASED_SECONDARY_ENABLE_EPT ) )
    {
        // PDPTE0
        fieldValue = VmxRead( VMCS_GUEST_PDPTE0_FULL);
        LOG( "PDPTE0: 0x%X\n", fieldValue );

        // PDPTE1
        fieldValue = VmxRead( VMCS_GUEST_PDPTE1_FULL);
        LOG( "PDPTE1: 0x%X\n", fieldValue );

        // PDPTE2
        fieldValue =VmxRead( VMCS_GUEST_PDPTE2_FULL);
        LOG( "PDPTE2: 0x%X\n", fieldValue );

        // PDPTE3
        fieldValue =VmxRead( VMCS_GUEST_PDPTE3_FULL);
        LOG( "PDPTE3: 0x%X\n", fieldValue );
    }
    else
    {
        LOG( "We don't have EPT :(\n" );
    }

    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.SecondaryProcessorBasedControls.AllowedOneSetting, PROC_BASED_SECONDARY_VIRTUAL_INTERRUPT_DELIVERY ) )
    {
        // Guest Interrupt Status
        fieldValue =VmxRead( VMCS_GUEST_INT_STATUS);
        LOG( "Guest Interrupt Status: 0x%X\n", fieldValue );
    }

#pragma endregion

}


static
void
DumpCurrentVmcsHostState(
    void
    )
{
    QWORD fieldValue;

    LOG( "-------------------------------------\n" );
    LOG( "Host-State Area\n")
    LOG( "-------------------------------------\n" );

    fieldValue = 0;

    // CR0
    fieldValue = VmxRead( VMCS_HOST_CR0);
    LOG( "CR0: 0x%X\n", fieldValue );

    // CR3
    fieldValue = VmxRead( VMCS_HOST_CR3);
    LOG( "CR3: 0x%X\n", fieldValue );

    // CR4
    fieldValue = VmxRead( VMCS_HOST_CR4);
    LOG( "CR4: 0x%X\n", fieldValue );

    // RSP
    fieldValue = VmxRead( VMCS_HOST_RSP);
    LOG( "RSP: 0x%X\n", fieldValue );

    // RIP
    fieldValue = VmxRead( VMCS_HOST_RIP);
    LOG( "RIP: 0x%X\n", fieldValue );

    // CS
    // CS.Selector
    fieldValue = VmxRead( VMCS_HOST_CS_SELECTOR);
    LOG( "CS.Selector: 0x%X\n", fieldValue );

    // SS
    // SS.Selector
    fieldValue = VmxRead( VMCS_HOST_SS_SELECTOR);
    LOG( "SS.Selector: 0x%X\n", fieldValue );

    // DS
    // DS.Selector
    fieldValue = VmxRead( VMCS_HOST_DS_SELECTOR);
    LOG( "DS.Selector: 0x%X\n", fieldValue );

    // ES
    // ES.Selector
    fieldValue = VmxRead( VMCS_HOST_ES_SELECTOR);
    LOG( "ES.Selector: 0x%X\n", fieldValue );

    // FS
    // FS.Selector
    fieldValue = VmxRead( VMCS_HOST_FS_SELECTOR);
    LOG( "FS.Selector: 0x%X\n", fieldValue );

    // FS.Address
    fieldValue = VmxRead( VMCS_HOST_FS_BASE);
    LOG( "FS.Address: 0x%X\n", fieldValue );

    // GS
    // GS.Selector
    fieldValue = VmxRead( VMCS_HOST_GS_SELECTOR);
    LOG( "GS.Selector: 0x%X\n", fieldValue );

    // GS.Address
    fieldValue = VmxRead( VMCS_HOST_GS_BASE);
    LOG( "GS.Address: 0x%X\n", fieldValue );

    // TR
    // TR.Selector
    fieldValue = VmxRead( VMCS_HOST_TR_SELECTOR);
    LOG( "TR.Selector: 0x%X\n", fieldValue );

    // TR.Address
    fieldValue = VmxRead( VMCS_HOST_TR_BASE);
    LOG( "TR.Address: 0x%X\n", fieldValue );

    // GDTR
    // GDTR.Address
    fieldValue = VmxRead( VMCS_HOST_GDTR_BASE);
    LOG( "GDTR.Address: 0x%X\n", fieldValue );

    // IDTR
    // IDTR.Address
    fieldValue = VmxRead( VMCS_HOST_IDTR_BASE);
    LOG( "IDTR.Address: 0x%X\n", fieldValue );

    LOG( "\nMSRs:\n" );
    // IA32_SYSENTER_CS
    fieldValue = VmxRead( VMCS_HOST_IA32_SYSENTER_CS);
    LOG( "IA32_SYSENTER_CS: 0x%X\n", fieldValue );

    // IA32_SYSENTER_ESP
    fieldValue = VmxRead( VMCS_HOST_IA32_SYSENTER_ESP);
    LOG( "IA32_SYSENTER_ESP: 0x%X\n", fieldValue );

    // IA32_SYSENTER_EIP
    fieldValue = VmxRead( VMCS_HOST_IA32_SYSENTER_EIP);
    LOG( "IA32_SYSENTER_EIP: 0x%X\n", fieldValue );

    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.VmxExitControls.AllowedOneSetting, EXIT_CONTROL_LOAD_IA32_PERF_GLOBAL_CTRL ) )
    {
        // IA32_PERF_GLOBAL_CONTROL
        fieldValue = VmxRead( VMCS_HOST_IA32_PERF_GLOBAL_CTRL_FULL);
        LOG( "IA32_PERF_GLOBAL_CTRL: 0x%X\n", fieldValue );
    }
    else
    {
        LOG( "We don't have load IA32_PERF_GLOBAL_CTRL :(\n" );
    }

    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.VmxExitControls.AllowedOneSetting, EXIT_CONTROL_LOAD_IA32_PAT ) )
    {
        // IA32_PAT
        fieldValue = VmxRead( VMCS_HOST_IA32_PAT_FULL);
        LOG( "IA32_PAT: 0x%X\n", fieldValue );
    }
    else
    {
        LOG( "We don't have load IA32_PAT :(\n" );
    }

    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.VmxExitControls.AllowedOneSetting, EXIT_CONTROL_LOAD_IA32_EFER ) )
    {
        // IA32_EFER
        fieldValue = VmxRead( VMCS_HOST_IA32_EFER_FULL);
        LOG( "IA32_EFER: 0x%X\n", fieldValue );
    }
    else
    {
        LOG( "We don't have load IA32_EFER :(\n" );
    }

}

static
void
DumpCurrentVmcsControlFields(
    void
    )
{
    // VM-Execution Control Fields

    DumpCurrentVmcsExecutionControlFields();

    // VM-Exit Control Fields

    DumpCurrentVmcsExitControlFields();

    // VM-Entry Control Fields

    DumpCurrentVmcsEntryControlFields();
}

static
void
DumpCurrentVmcsExecutionControlFields(
    void
    )
{
    QWORD fieldValue;

    LOG( "-------------------------------------\n" );
    LOG( "Execution Control Fields\n")
    LOG( "-------------------------------------\n" );

    fieldValue = 0;

    // Pin-Based VM-Execution Controls
    fieldValue = VmxRead( VMCS_CONTROL_PINBASED_CONTROLS);
    LOG( "Pin-Based Controls: 0x%X\n", fieldValue );

    // Primary Processor-Based VM-Execution Controls
    fieldValue = VmxRead( VMCS_CONTROL_PRIMARY_PROCBASED_CONTROLS);
    LOG( "Primary Processor-Based Controls: 0x%X\n", fieldValue );

    // Secondary Processor-Based VM-Execution Controls
    fieldValue = VmxRead( VMCS_CONTROL_SECONDARY_PROCBASED_CONTROLS);
    LOG( "Secondary Processor-Based Controls: 0x%X\n", fieldValue );

    // Exception Bitmap
    fieldValue = VmxRead( VMCS_CONTROL_EXCEPTION_BITMAP);
    LOG( "Exception Bitmap: 0x%X\n", fieldValue );

    // IO Bitmaps
    LOG( "IO Bitmaps:\n" );
    fieldValue = VmxRead( VMCS_CONTROL_IO_BITMAP_A_ADDRESS_FULL);
    LOG( "A.Bitmap: 0x%X\n", fieldValue );

    fieldValue = VmxRead( VMCS_CONTROL_IO_BITMAP_B_ADDRESS_FULL);
    LOG( "B.Bitmap: 0x%X\n", fieldValue );

    // TSC Offset
    fieldValue = VmxRead( VMCS_CONTROL_TSC_OFFSET_FULL);
    LOG( "TSC Offset: 0x%X\n", fieldValue );

    // CR0
    LOG( "\nCR0:\n" );
    // CR0.Mask
    fieldValue = VmxRead( VMCS_CONTROL_CR0_MASK);
    LOG( "Mask: 0x%X\n", fieldValue );

    // CR0.ReadShadow
    fieldValue = VmxRead( VMCS_CONTROL_CR0_READ_SHADOW);
    LOG( "Read Shadow: 0x%X\n", fieldValue );

    // CR4
    LOG( "\nCR4:\n" );
    // CR4.Mask
    fieldValue = VmxRead( VMCS_CONTROL_CR4_MASK);
    LOG( "Mask: 0x%X\n", fieldValue );

    // CR4.ReadShadow
    fieldValue = VmxRead( VMCS_CONTROL_CR4_READ_SHADOW);
    LOG( "Read Shadow: 0x%X\n", fieldValue );

    // CR3
    LOG( "\nCR3:\n" );
    // CR3.TargetCount
    fieldValue = VmxRead( VMCS_CONTROL_CR3_TARGET_COUNT);
    LOG( "TargetCount: 0x%X\n", fieldValue );

    // CR3.Target0
    fieldValue = VmxRead( VMCS_CONTROL_CR3_TARGET_0);
    LOG( "Target0: 0x%X\n", fieldValue );

    // CR3.Target1
    fieldValue = VmxRead( VMCS_CONTROL_CR3_TARGET_1);
    LOG( "Target1: 0x%X\n", fieldValue );

    // CR3.Target2
    fieldValue = VmxRead( VMCS_CONTROL_CR3_TARGET_2);
    LOG( "Target2: 0x%X\n", fieldValue );

    // CR3.Target3
    fieldValue = VmxRead( VMCS_CONTROL_CR3_TARGET_3);
    LOG( "Target3: 0x%X\n", fieldValue );

    LOG( "\nAPIC Virtualization Controls:\n" );

    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.SecondaryProcessorBasedControls.AllowedOneSetting, PROC_BASED_SECONDARY_VIRTUALIZE_APIC_ACCESS ) )
    {
        // APIC-access address
        fieldValue = VmxRead( VMCS_CONTROL_APIC_ACCESS_ADDRESS_FULL);
        LOG( "APIC-access address: 0x%X\n", fieldValue );
    }
    else
    {
        LOG( "We don't have virtualize APIC accesses :(\n")
    }

    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.PrimaryProcessorBasedControls.AllowedOneSetting, PROC_BASED_PRIMARY_USE_TPR_SHADOW ) )
    {
        // Virtual-APIC address
        fieldValue = VmxRead( VMCS_CONTROL_VIRTUAL_APIC_ADDRESS_FULL);
        LOG( "Virtual-APIC address: 0x%X\n", fieldValue );

        // TPR threshold
        fieldValue = VmxRead( VMCS_CONTROL_TPR_THRESHOLD);
        LOG( "TPR threshold: 0x%X\n", fieldValue );
    }
    else
    {
        LOG( "No TPR shadow :(\n" );
    }

    // EOI-exit bitmap
    LOG( "\nEOI-exit bitmap:\n" );

    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.SecondaryProcessorBasedControls.AllowedOneSetting, PROC_BASED_SECONDARY_VIRTUAL_INTERRUPT_DELIVERY ) )
    {
        // EOI_EXIT0
        fieldValue = VmxRead( VMCS_CONTROL_EOI_EXIT0_BITMAP_FULL);
        LOG( "EOI_EXIT0: 0x%X\n", fieldValue );

        // EOI_EXIT1
        fieldValue = VmxRead( VMCS_CONTROL_EOI_EXIT1_BITMAP_FULL);
        LOG( "EOI_EXIT1: 0x%X\n", fieldValue );

        // EOI_EXIT2
        fieldValue = VmxRead( VMCS_CONTROL_EOI_EXIT2_BITMAP_FULL);
        LOG( "EOI_EXIT2: 0x%X\n", fieldValue );

        // EOI_EXIT3
        fieldValue = VmxRead( VMCS_CONTROL_EOI_EXIT3_BITMAP_FULL);
        LOG( "EOI_EXIT3: 0x%X\n", fieldValue );
    }
    else
    {
        LOG( "EOI bitmaps not available :(\n" );
    }

    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.PinBasedControls.AllowedOneSetting, PIN_BASED_PROCESS_POSTED_INTS ) )
    {
        // Posted-interrupt notification vector
        fieldValue = VmxRead( VMCS_CONTROL_POSTED_INTERRUPT_NOTIFY_VECTOR);
        LOG( "Posted-interrupt notification vector: 0x%X\n", fieldValue );

        // Posted-interrupt descriptor address
        fieldValue = VmxRead( VMCS_CONTROL_POSTED_INT_DESC_ADDRESS_FULL);
        LOG( "Posted-interrupt descriptor address: 0x%X\n", fieldValue );
    }

    LOG( "\nMSR-Bitmap Address:\n" );
    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.PrimaryProcessorBasedControls.AllowedOneSetting, PROC_BASED_PRIMARY_USE_MSR_BITMAPS ) )
    {
        // MSR bitmap address
        fieldValue = VmxRead( VMCS_CONTROL_MSR_BITMAP_ADDRESS_FULL);
        LOG( "MSR bitmap address: 0x%X\n", fieldValue );
    }
    else
    {
        LOG( "MSR-Bitmaps not available :(\n" );
    }

    // Executive-VMCS Pointer
    fieldValue = VmxRead( VMCS_CONTROL_EXECUTIVE_VMCS_POINTER_FULL);
    LOG( "Executive-VMCS Pointer: 0x%X\n", fieldValue );

    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.SecondaryProcessorBasedControls.AllowedOneSetting, PROC_BASED_SECONDARY_ENABLE_EPT ) )
    {
        // EPTP
        fieldValue = VmxRead( VMCS_CONTROL_EPT_POINTER_FULL);
        LOG( "EPTP: 0x%X\n", fieldValue );
    }
    else
    {
        LOG( "We don't have EPT :(\n" );
    }

    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.SecondaryProcessorBasedControls.AllowedOneSetting, PROC_BASED_SECONDARY_ENABLE_VPID ) )
    {
        // VPID
        fieldValue = VmxRead(VMCS_CONTROL_VPID_IDENTIFIER);
        LOG( "VPID: 0x%X\n", fieldValue );
    }
    else
    {
        LOG( "We don't have VPIDs :(\n" );
    }

    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.SecondaryProcessorBasedControls.AllowedOneSetting, PROC_BASED_SECONDARY_PAUSE_LOOP_EXIT ) )
    {
        // PLE_Gap
        fieldValue = VmxRead( VMCS_CONTROL_PLE_GAP);
        LOG( "PLE_Gap: 0x%X\n", fieldValue );

        // PLE_Window
        fieldValue = VmxRead( VMCS_CONTROL_PLE_WINDOW);
        LOG( "PLE_Window: 0x%X\n", fieldValue );
    }
    else
    {
        LOG( "We don't have Pause-Loop Exiting :(\n" );
    }

    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.SecondaryProcessorBasedControls.AllowedOneSetting, PROC_BASED_SECONDARY_ENABLE_VM_FUNCS ) )
    {
        // VM-function controls
        fieldValue = VmxRead( VMCS_CONTROL_VM_FUNC_CONTROLS_FULL);
        LOG( "VM-function controls: 0x%X\n", fieldValue );

        // TODO: list the functions
    }
    else
    {
        LOG( "We don't have VM functions :(\n" );
    }

    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.SecondaryProcessorBasedControls.AllowedOneSetting, PROC_BASED_SECONDARY_VMCS_SHADOWING ) )
    {
        // VMRead Bitmap Address
        fieldValue = VmxRead( VMCS_CONTROL_VMREAD_BITMAP_ADDRESS_FULL);
        LOG( "VMRead Bitmap Address: 0x%X\n", fieldValue );

        // VMWrite Bitmap Address
        fieldValue = VmxRead( VMCS_CONTROL_VMWRITE_BITMAP_ADDRESS_FULL);
        LOG( "VMWrite Bitmap Address: 0x%X\n", fieldValue );
    }
    else
    {
        LOG( "We don't have VMCS shadowing :(\n" );
    }

    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.SecondaryProcessorBasedControls.AllowedOneSetting, PROC_BASED_SECONDARY_EPT_VIOLATION_INTERRUPT ) )
    {
        // Virtualization-exception information address
        fieldValue = VmxRead( VMCS_CONTROL_VE_INFO_ADDRESS_FULL);
        LOG( "Virtualization-exception information addresss: 0x%X\n", fieldValue );

        // EPTP index
        fieldValue = VmxRead( VMCS_CONTROL_EPTP_INDEX);
        LOG( "EPTP index: 0x%X\n", fieldValue );
    }
    else
    {
        LOG( "We don't have EPT-violation #VE :(\n" );
    }

    if( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.SecondaryProcessorBasedControls.AllowedOneSetting, PROC_BASED_SECONDARY_ENABLE_XSAVES_XSTORS ) )
    {
        // XSS Exiting Bitmap
        fieldValue = VmxRead( VMCS_CONTROL_XSS_EXISTING_BITMAP_FULL);
        LOG( "XSS Exiting Bitmap: 0x%X\n", fieldValue );
    }
    else
    {
        LOG( "We don't have XSAVES/XRSTORS :(\n" );
    }
}

static
void
DumpCurrentVmcsExitControlFields(
    void
    )
{
    QWORD fieldValue;

    LOG( "-------------------------------------\n" );
    LOG( "Exit Control Fields\n")
    LOG( "-------------------------------------\n" );

    fieldValue = 0;

    // VM-Exit Controls
    fieldValue = VmxRead( VMCS_CONTROL_VM_EXIT_CONTROLS);
    LOG( "VM-Exit Controls: 0x%X\n", fieldValue );

    // VM-Exit MSR-store count
    fieldValue = VmxRead( VMCS_CONTROL_VM_EXIT_MSR_STORE_COUNT);
    LOG( "MSR-store count: 0x%X\n", fieldValue );

    // VM-Exit MSR-store address
    fieldValue = VmxRead( VMCS_CONTROL_MSR_STORE_EXIT_ADDRESS_FULL);
    LOG( "MSR-store address: 0x%X\n", fieldValue );

    // VM-Exit MSR-load count
    fieldValue = VmxRead( VMCS_CONTROL_VM_EXIT_MSR_LOAD_COUNT);
    LOG( "MSR-load count: 0x%X\n", fieldValue );

    // VM-Exit MSR-load address
    fieldValue = VmxRead( VMCS_CONTROL_MSR_LOAD_EXIT_ADDRESS_FULL);
    LOG( "MSR-load address: 0x%X\n", fieldValue );
}

static
void
DumpCurrentVmcsEntryControlFields(
    void
    )
{
    QWORD fieldValue;

    LOG( "-------------------------------------\n" );
    LOG( "Entry Control Fields\n")
    LOG( "-------------------------------------\n" );

    fieldValue = 0;

    // VM-Entry Controls
    fieldValue = VmxRead( VMCS_CONTROL_VM_ENTRY_CONTROLS);
    LOG( "VM-Entry Controls: 0x%X\n", fieldValue );

    // VM-Entry MSR-load count
    fieldValue = VmxRead( VMCS_CONTROL_VM_ENTRY_MSR_LOAD_COUNT);
    LOG( "MSR-load count: 0x%X\n", fieldValue );

    // VM-Entry MSR-load address
    fieldValue = VmxRead( VMCS_CONTROL_MSR_LOAD_ENTRY_ADDRESS_FULL);
    LOG( "MSR-load address: 0x%X\n", fieldValue );

    // VM-Entry Controls for Event Injection

    // VM-Entry interruption-information field
    fieldValue = VmxRead( VMCS_CONTROL_VM_ENTRY_INT_INFO_FIELD);
    LOG( "VM-Entry interruption-information field: 0x%X\n", fieldValue );

    // VM-Entry exception error code
    fieldValue = VmxRead( VMCS_CONTROL_VM_ENTRY_EXCEPTION_ERROR_CODE);
    LOG( "VM-Entry exception error code: 0x%X\n", fieldValue );

    // VM-Entry instruction length
    fieldValue = VmxRead( VMCS_CONTROL_VM_ENTRY_INSTRUCTION_LENGTH);
    LOG( "VM-Entry instruction length: 0x%X\n", fieldValue );
}
#pragma warning(pop)
