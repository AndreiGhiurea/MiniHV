#include "vmguest.h"
#include "vmcs.h"
#include "dmp_cpu.h"
#include "native/memory.h"
#include "data.h"

#pragma warning(push)

// warning C28039: The type of actual parameter '24576|2048|((0<<1))|0' should exactly match the type 'VMCS_FIELD':
#pragma warning(disable: 28039)
QWORD
GuestRetrieveGeneralPurposeRegisterValue(
    IN      BYTE        RegisterIndex
    )
{
    ASSERT(RegisterIndex <= RegisterR15);

    return GetCurrentVcpu()->ProcessorState->RegisterArea.RegisterValues[RegisterIndex];
}

QWORD
GuestRetrieveControlRegisterValue(
    IN      BYTE        RegisterIndex
    )
{
    VMCS_FIELD fieldToRead = VMCS_GUEST_CR0;

    switch( RegisterIndex )
    {
    case CR0:
        fieldToRead = VMCS_GUEST_CR0;
        break;
    case CR3:
        fieldToRead = VMCS_GUEST_CR3;
        break;
    case CR4:
        fieldToRead = VMCS_GUEST_CR4;
        break;
    default:
        NOT_REACHED;
    }

    return VmxRead(fieldToRead);
}

QWORD
GuestRetrieveMsrValue(
    IN      DWORD       MsrIndex
    )
{
    switch(MsrIndex)
    {
        case IA32_PERF_GLOBAL_CTRL:
            return VmxRead(VMCS_GUEST_IA32_PERF_GLOBAL_CTRL_FULL);
        case IA32_EFER:
            return VmxRead(VMCS_GUEST_IA32_EFER_FULL);
        case IA32_PAT:
            return VmxRead(VMCS_GUEST_IA32_PAT_FULL);
        case IA32_DEBUGCTL:
            return VmxRead(VMCS_GUEST_IA32_DEBUGCTL_FULL);
        case IA32_SYSENTER_EIP:
            return VmxRead(VMCS_GUEST_IA32_SYSENTER_EIP);
        case IA32_SYSENTER_ESP:
            return VmxRead(VMCS_GUEST_IA32_SYSENTER_ESP);
        case IA32_SYSENTER_CS:
            return VmxRead(VMCS_GUEST_IA32_SYSENTER_CS);
        default:
            return __readmsr(MsrIndex);
    }
}

typedef struct _VMCS_FIELDS_FOR_SELECTOR
{
    VMCS_FIELD                      BaseField;

    VMCS_FIELD                      AccessRightsField;
    VMCS_FIELD                      LimitField;

    VMCS_FIELD                      SelectorField;
} VMCS_FIELDS_FOR_SELECTOR;

static const VMCS_FIELDS_FOR_SELECTOR SELECTOR_FIELDS[] =
{
    {VMCS_GUEST_ES_BASE, VMCS_GUEST_ES_ACCESS_RIGHTS, VMCS_GUEST_ES_LIMIT, VMCS_GUEST_ES_SELECTOR}, //SelectorES,
    {VMCS_GUEST_CS_BASE, VMCS_GUEST_CS_ACCESS_RIGHTS, VMCS_GUEST_CS_LIMIT, VMCS_GUEST_CS_SELECTOR}, //SelectorCS,
    {VMCS_GUEST_SS_BASE, VMCS_GUEST_SS_ACCESS_RIGHTS, VMCS_GUEST_SS_LIMIT, VMCS_GUEST_SS_SELECTOR}, //SelectorSS,
    {VMCS_GUEST_DS_BASE, VMCS_GUEST_DS_ACCESS_RIGHTS, VMCS_GUEST_DS_LIMIT, VMCS_GUEST_DS_SELECTOR}, //SelectorDS,
    {VMCS_GUEST_FS_BASE, VMCS_GUEST_FS_ACCESS_RIGHTS, VMCS_GUEST_FS_LIMIT, VMCS_GUEST_FS_SELECTOR}, //SelectorFS,
    {VMCS_GUEST_GS_BASE, VMCS_GUEST_GS_ACCESS_RIGHTS, VMCS_GUEST_GS_LIMIT, VMCS_GUEST_GS_SELECTOR}, //SelectorGS,
    {VMCS_GUEST_TR_BASE, VMCS_GUEST_TR_ACCESS_RIGHTS, VMCS_GUEST_TR_LIMIT, VMCS_GUEST_TR_SELECTOR}, //SelectorTR,
};
STATIC_ASSERT(ARRAYSIZE(SELECTOR_FIELDS) == SelectorReserved);

__forceinline
static
BOOLEAN
_IsSelectorIndexValid(
    IN      SelectorIndex           Index
    )
{
    return ((SelectorFirst <= Index) && (Index < SelectorReserved));
}

STATUS
GuestRetrieveSelectorInfo(
    IN      SelectorIndex           Index,
    OUT     GUEST_SELECTOR_INFO*    SelectorInfo
    )
{
    const VMCS_FIELDS_FOR_SELECTOR* pFieldsToRead;

    ASSERT(_IsSelectorIndexValid(Index));
    ASSERT(SelectorInfo != NULL);

    pFieldsToRead = &SELECTOR_FIELDS[Index];

    SelectorInfo->BaseAddress   = VmxRead(pFieldsToRead->BaseField);
    SelectorInfo->AccessRights  = (DWORD) VmxRead(pFieldsToRead->AccessRightsField);
    SelectorInfo->Limit         = (DWORD) VmxRead(pFieldsToRead->LimitField);
    SelectorInfo->Selector      = (WORD) VmxRead(pFieldsToRead->SelectorField);

    return STATUS_SUCCESS;
}

STATUS
GuestRetrieveAllSelectorsInfo(
    OUT_WRITES_ALL(SelectorReserved)
            GUEST_SELECTOR_INFO*    SelectorsInfo
    )
{
    for (SelectorIndex sel = SelectorFirst; sel < SelectorTR; ++sel)
    {
        STATUS status = GuestRetrieveSelectorInfo(sel, &SelectorsInfo[sel]);
        if (!SUCCEEDED(status)) return status;
    }

    return STATUS_SUCCESS;
}

STATUS
GuestUpdateSelectorInfo(
    IN      SelectorIndex           Index,
    IN      GUEST_SELECTOR_INFO*    SelectorInfo
    )
{
    const VMCS_FIELDS_FOR_SELECTOR* pFieldsToWrite;

    ASSERT(_IsSelectorIndexValid(Index));
    ASSERT(SelectorInfo != NULL);

    pFieldsToWrite = &SELECTOR_FIELDS[Index];

    VmxWrite(pFieldsToWrite->BaseField, SelectorInfo->BaseAddress);
    VmxWrite(pFieldsToWrite->AccessRightsField, SelectorInfo->AccessRights);
    VmxWrite(pFieldsToWrite->LimitField, SelectorInfo->Limit);
    VmxWrite(pFieldsToWrite->SelectorField, SelectorInfo->Selector);

    return STATUS_SUCCESS;
}

STATUS
GuestUpdateAllSelectorsInfo(
    IN_READS(SelectorReserved)
    GUEST_SELECTOR_INFO*    SelectorsInfo
    )
{
    for (SelectorIndex sel = SelectorFirst; sel < SelectorTR; ++sel)
    {
        STATUS status = GuestUpdateSelectorInfo(sel, &SelectorsInfo[sel]);
        if (!SUCCEEDED(status)) return status;
    }

    return STATUS_SUCCESS;
}

STATUS
GuestVAToHostPA(
    IN      PVOID       GuestVa,
    OUT_PTR PVOID*      HostPa
    )
{
    QWORD guestCr0;
    QWORD guestCr3;
    QWORD guestCr4;
    QWORD guestEfer;

    if( NULL == HostPa )
    {
        return STATUS_INVALID_PARAMETER2;
    }

    guestCr0 = 0;
    guestEfer = 0;

    guestCr0 = VmxRead(VMCS_GUEST_CR0);

    if( !IsBooleanFlagOn( guestCr0, CR0_PG ) )
    {
        // if no paging => guest VA == guest PA
        *HostPa = (PVOID) GPA2HPA( GuestVa );
        return STATUS_SUCCESS;
    }

    // if we're here it's clear guest has paging set
    // and the NULL pointer becomes invalid
    if (NULL == GuestVa)
    {
        return STATUS_INVALID_PARAMETER1;
    }

    // need to check for the 3 paging modes

    guestEfer = VmxRead(VMCS_GUEST_IA32_EFER_FULL);

    guestCr3 = VmxRead(VMCS_GUEST_CR3);

    if( IsBooleanFlagOn( guestEfer, IA32_EFER_LMA ) )
    {
        // 64-bit paging is activated
        *HostPa = VA64toPA( ( PML4* ) &guestCr3, GuestVa );
        if( NULL == *HostPa )
        {
            LOGL("VA64toPA failed\n");
            return STATUS_VMX_GUEST_MEMORY_CANNOT_BE_MAPPED;
        }

        return STATUS_SUCCESS;
    }

    guestCr4 = VmxRead(VMCS_GUEST_CR4);

    // PAE or 32-bit paging
    if( IsBooleanFlagOn( guestCr4, CR4_PAE ) )
    {
        // PAE paging
        *HostPa = VAPAEtoPA((CR3_PAE_STRUCTURE*)&guestCr3, GuestVa);
        if (NULL == *HostPa)
        {
            LOGL("VAPAEtoPA failed\n");
            return STATUS_VMX_GUEST_MEMORY_CANNOT_BE_MAPPED;
        }

        return STATUS_SUCCESS;
    }

    // we're here => 32-bit paging
    *HostPa = VA32toPA((CR3_STRUCTURE*)&guestCr3, GuestVa);
    if (NULL == *HostPa)
    {
        LOGL("VA32toPA failed\n");
        return STATUS_VMX_GUEST_MEMORY_CANNOT_BE_MAPPED;
    }

    return STATUS_SUCCESS;
}

STATUS
GuestVAToHostVA(
    IN          PVOID       GuestVa,
    OUT_OPT_PTR PVOID*      HostPa,
    OUT_PTR     PVOID*      HostVa
    )
{
    STATUS status;
    PVOID pMappedAddress;
    PVOID hostPa;

    if( NULL == HostVa )
    {
        return STATUS_INVALID_PARAMETER3;
    }

    status = STATUS_SUCCESS;
    hostPa = NULL;
    pMappedAddress = NULL;

    status = GuestVAToHostPA( GuestVa, &hostPa );
    if( !SUCCEEDED( status ) )
    {
        LOGP( "GuestVAToHostPA failed for GuestVa: 0x%X\n", GuestVa );
        return status;
    }

    pMappedAddress = MapMemory( hostPa, PAGE_SIZE );
    if (NULL == pMappedAddress)
    {
        LOGP( "MapMemory failed for GuestVa: 0x%X\n", GuestVa );
        return STATUS_MEMORY_CANNOT_BE_MAPPED;
    }

    // if we're here we succeeded => we can set the output
    if (NULL != HostPa)
    {
        *HostPa = hostPa;
    }
    *HostVa = pMappedAddress;

    return status;
}

STATUS
GuestInjectEvent(
    IN      BYTE        VectorIndex,
    IN      BYTE        EventType,
    IN_OPT  DWORD*      ErrorCode
    )
{
    VM_ENTRY_INT_INFO intInfo;

    memzero( &intInfo, sizeof(VM_ENTRY_INT_INFO) );

    // we need an interlocked operation because we could be treating a normal
    // interrupt (non-NMI) and even though IF = 0 a NMI can come
    ASSERT(1 == _InterlockedIncrement(&(GetCurrentVcpu()->PendingEventForInjection)));

    intInfo.Valid = 1;
    intInfo.Vector = VectorIndex;
    intInfo.InterruptionType = EventType & 0x7;

    if( NULL != ErrorCode )
    {
        intInfo.ErrorCode = 1;

        LOGP( "About to set error code: 0x%x\n", *ErrorCode );

        ASSERT_INFO( *ErrorCode <= 0x7FFF, "26.2.1 states that if deliver error code bit is set => VM-entry exception error-code[31:15] = 0")

        VmxWrite( VMCS_CONTROL_VM_ENTRY_EXCEPTION_ERROR_CODE, *ErrorCode );
    }

    // the instruction length doesn't need to be set
    // Intel vol.3: VM-entry instruction length (32 bits). For injection of events whose type is software interrupt, software
    // exception, or privileged software exception, this field is used to determine the value of RIP that is pushed on
    // the stack.
    ASSERT( ( InterruptionTypeHardwareException == EventType ) || ( InterruptionTypeNMI == EventType ) );

    VmxWrite( VMCS_CONTROL_VM_ENTRY_INT_INFO_FIELD, intInfo.Raw );

    return STATUS_SUCCESS;
}

STATUS
GuestMapMemoryRange(
    IN_OPT  PVOID       StartingAddress,
    IN      QWORD       SizeOfMemoryToMap
    )
{
    STATUS status;
    BYTE memoryType;
    PVOID eptMappingResult;
    BYTE* endAddress;
    BYTE* addressToMap;

    if( 0 == SizeOfMemoryToMap )
    {
        return STATUS_INVALID_PARAMETER2;
    }

    endAddress = (BYTE*) AlignAddressUpper( (BYTE*)StartingAddress + SizeOfMemoryToMap, PAGE_SIZE );

    for (addressToMap = StartingAddress; addressToMap < endAddress; addressToMap = addressToMap + PAGE_SIZE)
    {
        status = MtrrFindPhysicalAddressMemoryType( ( PVOID ) addressToMap, &memoryType );
        if( !SUCCEEDED( status ) )
        {
            LOGL("MtrrFindPhysicalAddressMemoryType failed with status 0x%x for address: 0x%X\n", status, addressToMap);
            return status;
        }

        if( MEMORY_TYPE_WRITEBACK != memoryType )
        {
            memoryType = MEMORY_TYPE_STRONG_UNCACHEABLE;
        }

        /// TODO: map through EPT
        eptMappingResult = (void*)1; /* EptMapGuestPA((PVOID)addressToMap, PAGE_SIZE, memoryType, NULL, MAX_BYTE, FALSE, FALSE);*/

        // we can receive NULL in case addressToMap is NULL
        if( ( NULL == eptMappingResult ) && ( NULL != addressToMap ) )
        {
            LOGL("EptMapGuestPA failed for address: 0x%X\n", addressToMap);
            return STATUS_VMX_EPT_MAPPING_FAILED;
        }

        eptMappingResult = NULL;
    }

    return STATUS_SUCCESS;
}
#pragma warning(pop)
