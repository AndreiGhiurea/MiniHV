#ifndef _VM_GUEST_H_
#define _VM_GUEST_H_

#include "minihv.h"
#include "cpumu.h"
#include "segment.h"
#include "idt_handlers.h"


// 24.4 Guest State Area

// 24.4.1 Guest Register State

// We will use "unrestricted guest" => we don't need
// PG and PE
#define VM_GUEST_CR0                                ( CR0_CD | CR0_NW | CR0_NE | CR0_ET)
#define VM_GUEST_CR3                                0
#define VM_GUEST_CR4                                ( CR4_VMXE )

#define VM_GUEST_DR7                                0x400

// we know that we have free memory under 0x7C00
#define VM_GUEST_RSP                                0x7C00

// This will contain address of the under 1Mb piece of code
#define VM_GUEST_RIP                                0x9E00

// Reserved bit must be set
#define VM_GUEST_RFLAGS                             (RFLAGS_RESERVED_BIT)

// CS
#define VM_GUEST_CS_SELECTOR                        0
#define VM_GUEST_CS_ADDRESS                         0
#define VM_GUEST_CS_SEG_LIMIT                       MAX_WORD
#define VM_GUEST_CS_ACCESS_RIGHTS                   ( SEGMENT_ACCESS_RIGHTS_P_BIT | SEGMENT_ACCESS_RIGHTS_DESC_TYPE | CODE_SEGMENT_EXECUTE_ONLY_ACCESSED )

// SS
#define VM_GUEST_SS_SELECTOR                        0
#define VM_GUEST_SS_ADDRESS                         0
#define VM_GUEST_SS_SEG_LIMIT                       MAX_WORD
#define VM_GUEST_SS_ACCESS_RIGHTS                   ( SEGMENT_ACCESS_RIGHTS_P_BIT | SEGMENT_ACCESS_RIGHTS_DESC_TYPE | DATA_SEGMENT_READ_WRITE_ACCESSED )

// DS
#define VM_GUEST_DS_SELECTOR                        0
#define VM_GUEST_DS_ADDRESS                         0
#define VM_GUEST_DS_SEG_LIMIT                       MAX_WORD
#define VM_GUEST_DS_ACCESS_RIGHTS                   ( SEGMENT_ACCESS_RIGHTS_P_BIT | SEGMENT_ACCESS_RIGHTS_DESC_TYPE | DATA_SEGMENT_READ_WRITE_ACCESSED )

// ES
#define VM_GUEST_ES_SELECTOR                        0
#define VM_GUEST_ES_ADDRESS                         0
#define VM_GUEST_ES_SEG_LIMIT                       MAX_WORD
#define VM_GUEST_ES_ACCESS_RIGHTS                   ( SEGMENT_ACCESS_RIGHTS_P_BIT | SEGMENT_ACCESS_RIGHTS_DESC_TYPE | DATA_SEGMENT_READ_WRITE_ACCESSED )

// FS
#define VM_GUEST_FS_SELECTOR                        0
#define VM_GUEST_FS_ADDRESS                         0
#define VM_GUEST_FS_SEG_LIMIT                       MAX_WORD
#define VM_GUEST_FS_ACCESS_RIGHTS                   ( SEGMENT_ACCESS_RIGHTS_P_BIT | SEGMENT_ACCESS_RIGHTS_DESC_TYPE | DATA_SEGMENT_READ_WRITE_ACCESSED )

// GS
#define VM_GUEST_GS_SELECTOR                        0
#define VM_GUEST_GS_ADDRESS                         0
#define VM_GUEST_GS_SEG_LIMIT                       MAX_WORD
#define VM_GUEST_GS_ACCESS_RIGHTS                   ( SEGMENT_ACCESS_RIGHTS_P_BIT | SEGMENT_ACCESS_RIGHTS_DESC_TYPE | DATA_SEGMENT_READ_WRITE_ACCESSED )

// LDTR
#define VM_GUEST_LDTR_SELECTOR                      0
#define VM_GUEST_LDTR_ADDRESS                       0
#define VM_GUEST_LDTR_SEG_LIMIT                     0
#define VM_GUEST_LDTR_ACCESS_RIGHTS                 ( SEGMENT_ACCESS_RIGHTS_P_BIT | LDT_SEGMENT )

// TR
#define VM_GUEST_TR_SELECTOR                        0
#define VM_GUEST_TR_ADDRESS                         0
#define VM_GUEST_TR_SEG_LIMIT                       0x67
#define VM_GUEST_TR_ACCESS_RIGHTS                   ( SEGMENT_ACCESS_RIGHTS_P_BIT | TSS_SEGMENT_16_BIT_BUSY )

// GDTR
#define VM_GUEST_GDTR_BASE_ADDRESS                  0
#define VM_GUEST_GDTR_LIMIT                         0

// IDTR
#define VM_GUEST_IDTR_BASE_ADDRESS                  0
#define VM_GUEST_IDTR_LIMIT                         IVT_LIMIT

// MSRs
#define VM_GUEST_IA32_DEBUGCTL                      0
#define VM_GUEST_IA32_SYSENTER_CS                   0
#define VM_GUEST_IA32_SYSENTER_ESP                  0
#define VM_GUEST_IA32_SYSENTER_EIP                  0

// Needs only to be set if
// "load IA32_PERF_GLOBAL_CTRL" is set
#define VM_GUEST_IA32_PERF_GLOBAL_CTRL              0

// Only if "load IA32_PAT" or "save IA32_PAT" is set
// we set it to our PAT
//#define VM_GUEST_IA32_PAT                           0

// Only if "load IA32_EFER" or "save IA32_EFER" is set
#define VM_GUEST_IA32_EFER                          0

// SMBASE - Unmodified by all VM entries except those
// that return from SMM
#define VM_GUEST_SMBASE                             0

// 24.4.2 Guest Non-Register State

#define VM_GUEST_ACTIVITY_STATE_BSP                 ActivityStateActive

// we will actually use a short timer to wake up very early
// and increment the number of CPU's which performed the vmcs_launch
// this way BSP can enter last in operation
#define VM_GUEST_ACTIVITY_STATE_AP_NESTED           ActivityStateHlt
#define VM_GUEST_ACTIVITY_STATE_AP_WITH_TIMER       ActivityStateHlt
#define VM_GUEST_ACTIVITY_STATE_AP_NO_TIMER         ActivityStateWaitForSIPI
#define VM_GUEST_INTERRUPTIBILITY_STATE             0
#define VM_GUEST_PENDING_DEBUG_EXCEPTION            0

// if "VMCS shadowing" is set =>
// VMREAD and VMWRITE access the VMCS referenced by this pointer
// should be set to 0xFFFFFFFF_FFFFFFFF if clear
#define VM_GUEST_VMCS_LINK_POINTER                  MAX_QWORD

// if "activate VMX-preemption timer" is set
#define VM_GUEST_PREEMPT_TIMER_VALUE_NORMAL         (MAX_DWORD)
#define VM_GUEST_PREEMPT_TIMER_VALUE_STARTUP        (1UL)

// if "enable EPT" and PAE paging is used
#define VM_GUEST_PDPTE_S                            0

// need "virtual-interrupt delivery" support
#define VM_GUEST_INTERRUPT_STATUS                   0

// 24.5 Host State Area

#define VM_HOST_DATA_SEGMENT_SELECTOR               0x8
#define VM_HOST_CODE_SEGMENT_SELECTOR               0x10

// 24.6 VM-Execution Control Fields

// 24.6.1 Pin-Based VM-Execution Controls
// NOTE: These controls will be OR'ed with the necessary
// default1 bits
#define VM_CONTROL_PIN_BASED_CTLS                   0

// 24.6.2 Processor-Based VM-Execution Controls
// NOTE: These controls will be OR'ed with the necessary
// default1 bits
#define VM_CONTROL_PRIMARY_PROC_BASED_CTLS          (   PROC_BASED_PRIMARY_ACTIVATE_SECONDARY_CTLS \
                                                        )

// NOTE: These controls will be OR'ed with the necessary
// default1 bits
#define VM_CONTROL_SECONDARY_PROC_BASED_CTLS        (PROC_BASED_SECONDARY_ENABLE_EPT | PROC_BASED_SECONDARY_UNRESTRICTED_GUEST)

// 24.6.3 Exception Bitmap
#define VM_CONTROL_EXCEPTION_BITMAP_BSP             0
#define VM_CONTROL_EXCEPTION_BITMAP_AP              ((DWORD)1<<ExceptionGeneralProtection)
#define VM_CONTROL_EXCEPTION_BITMAP_AP_NESTED       0

// 24.6.6 Guest/Host Mask and Read Shadows for CR0 and CR4
// we only care if PE or PG changes
#define VM_CONTROL_CR0_MASK                         CR0_NE
#define VM_CONTROL_CR0_SHADOW                       CR0_NE

#define VM_CONTROL_CR4_MASK                         CR4_VMXE
#define VM_CONTROL_CR4_SHADOW                       CR4_VMXE

// 24.6.7 CR3-Target Controls
#define VM_CONTROL_CR3_TARGET_COUNT                 0

// 24.7 VM-Exit Control Fields

// 24.7.1 VM-Exit Controls
// 26.2.4 :) If the logical processor is in IA-32e mode at the time of
// VMEntry => "the host address-space size" must be 1
#define VM_CONTROL_EXIT_CTLS                        (   EXIT_CONTROL_LOAD_IA32_EFER                 | \
                                                        EXIT_CONTROL_SAVE_IA32_EFER                 | \
                                                        EXIT_CONTROL_LOAD_IA32_PAT                  | \
                                                        EXIT_CONTROL_SAVE_IA32_PAT                  | \
                                                        EXIT_CONTROL_HOST_ADDRESS_SPACE_SIZE        | \
                                                        EXIT_CONTROL_SAVE_DEBUG_CTLS                  \
                                                        )

// 24.8 VM-Entry Control Fields

// 24.8.1 VM-Entry Controls
#define VM_CONTROL_ENTRY_CTLS                       (   ENTRY_CONTROL_LOAD_IA32_EFER            | \
                                                        ENTRY_CONTROL_LOAD_IA32_PAT             | \
                                                        ENTRY_CONTROL_LOAD_DEBUG_CTLS             \
                                                        )

//******************************************************************************
// Function:    GuestRetrieveGeneralPurposeRegisterValue
// Description: Returns GPR value.
// Returns:     STATUS
// Parameter:   IN BYTE RegisterIndex
// Parameter:   OUT QWORD * RegisterValue
//******************************************************************************
QWORD
GuestRetrieveGeneralPurposeRegisterValue(
    IN      BYTE        RegisterIndex
    );

//******************************************************************************
// Function:    GuestRetrieveControlRegisterValue
// Description: Returns CR value.
// Returns:     STATUS
// Parameter:   IN BYTE RegisterIndex
// Parameter:   OUT QWORD * RegisterValue
//******************************************************************************
QWORD
GuestRetrieveControlRegisterValue(
    IN      BYTE        RegisterIndex
    );

QWORD
GuestRetrieveMsrValue(
    IN      DWORD       MsrIndex
    );

STATUS
GuestRetrieveSelectorInfo(
    IN      SelectorIndex           Index,
    OUT     GUEST_SELECTOR_INFO*    SelectorInfo
    );

STATUS
GuestRetrieveAllSelectorsInfo(
    OUT_WRITES_ALL(SelectorReserved)
            GUEST_SELECTOR_INFO*    SelectorsInfo
    );

STATUS
GuestUpdateSelectorInfo(
    IN      SelectorIndex           Index,
    IN      GUEST_SELECTOR_INFO*    SelectorInfo
    );

STATUS
GuestUpdateAllSelectorsInfo(
    IN_READS(SelectorReserved)
            GUEST_SELECTOR_INFO*    SelectorsInfo
    );

STATUS
GuestVAToHostPA(
    IN      PVOID       GuestVa,
    OUT_PTR PVOID*      HostPa
    );

STATUS
GuestVAToHostVA(
    IN          PVOID       GuestVa,
    OUT_OPT_PTR PVOID*      HostPa,
    OUT_PTR     PVOID*      HostVa
    );

STATUS
GuestInjectEvent(
    IN      BYTE        VectorIndex,
    IN      BYTE        EventType,
    IN_OPT  DWORD*      ErrorCode
    );

STATUS
GuestMapMemoryRange(
    IN_OPT  PVOID       StartingAddress,
    IN      QWORD       SizeOfMemoryToMap
    );

#endif // _VM_GUEST_H_