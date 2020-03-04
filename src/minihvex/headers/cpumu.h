#ifndef _CPU_H_
#define _CPU_H_

#include "minihv.h"
#include "list.h"
#include "vmx_operations.h"
#include "task.h"
#include "apic.h"
#include "cpu.h"
#include "vmexit_reason.h"
#include "idt.h"

// ECX
#define CPUID_FEATURE_ECX_HYPERVISOR_PRESENT_BIT    ((QWORD)1<<31)
#define CPUID_FEATURE_ECX_RDRAND                    ((QWORD)1<<30)
#define CPUID_FEATURE_ECX_OSXSAVE                   ((QWORD)1<<27)
#define CPUID_FEATURE_ECX_XSAVE                     ((QWORD)1<<26)
#define CPUID_FEATURE_ECX_X2APIC                    ((QWORD)1<<21)
#define CPUID_FEATURE_ECX_SMX                       ((QWORD)1<<6)
#define CPUID_FEATURE_ECX_VMX                       ((QWORD)1<<5)
#define CPUID_FEATURE_ECX_MONITOR                   ((QWORD)1<<3)

// physical CPU
typedef struct _PCPU
{
    BYTE            ApicID;             // Used for sending IPIs
    PVOID           StackBase;          // Pointer to the stack base
    BOOLEAN         BspProcessor;       // if TRUE => this is the BSP

    // these fields are used for VMX operation
    PVMXON_REGION   VmxOnRegion;

    // TR selector
    WORD            TrSelector;

    // TSS base address
    TSS*            TssAddress;

    // for the moment the MiniHV support only 1VCPU/PCPU
    // else we could have used a LL
    struct _VCPU*   VirtualCpu;

    // list of physical processors in the system
    LIST_ENTRY      ListEntry;
} PCPU, *PPCPU;

typedef struct _GUEST_SELECTOR_INFO
{
    QWORD               BaseAddress;

    DWORD               AccessRights;
    DWORD               Limit;

    WORD                Selector;
} GUEST_SELECTOR_INFO;

typedef struct _VCPU
{
    // the physical CPU to which the VCPU belongs to
    PCPU*                   PhysicalCpu;

    // pointer to the Vmcs Region
    PVMCS_REGION            VmcsRegion;

    // processor state before VM Exit
    COMPLETE_PROCESSOR_STATE*   ProcessorState;

    // SIPI didn't occur yet on the processor
    BOOLEAN                 WaitingForWakeup;

    // SIPI occurred, a #GP will occur on Windows x64 systems on APs
    BOOLEAN                 ExpectingGPAfterSIPI;

    // CPU has entered VMX mode
    BOOLEAN                 EnteredVMX;

    volatile DWORD          PendingEventForInjection;

    BYTE                    IcrHighApicId;
    BOOLEAN                 ReceivedSIPI;
} VCPU, *PVCPU;

// information about the CPU capabilities
typedef struct _FEATURE_DATA
{
    // true if the system supports X2APIC
    BOOLEAN         X2APICAvailable;

    BOOLEAN         XSaveSupport;

    BOOLEAN         MtrrSupport;

    BYTE            PhysicalAddressBits;
    QWORD           PhysicalAddressMask;

    BYTE            LinearAddressBits;
} FEATURE_DATA, *PFEATURE_DATA;

void
CpuMuCollectFeatures(
    void
    );

//******************************************************************************
// Function:    CheckFeatures
// Description: Checks if the system supports the necessary CPU features and
//              also checks for optional features which might optimize the code.
// Returns:     void
// Parameter:   void
//******************************************************************************
void
CpuMuCheckFeatures(
    void
    );


//******************************************************************************
// Function:    EnableFeatures
// Description: Enables all the optional features we've found.
// Returns:     void
// Parameter:   void
// NOTE:        EXPERIMENTAL!
//******************************************************************************
void
CpuMuEnableFeatures(
    void
    );

STATUS
CpuMuSetMonitorFilterSize(
    IN          WORD        FilterSize
    );

STATUS
CpuCreateTSSDescriptor(
    void
    );

STATUS
CpumuSetFpuFeatures(
    _In_        XCR0_SAVED_STATE        Features
    );

#define GetCurrentPcpu()  ((PCPU*)__readmsr(IA32_GS_BASE_MSR))
#define GetCurrentVcpu()  ((VCPU*)__readmsr(IA32_FS_BASE_MSR))

#define CURRENT_CPU_MASK        0x8000'0000'0000'0000ULL

__forceinline
extern
PVOID
CpuGetCurrent(void)
{
    // warning C4306: 'type cast': conversion from 'BYTE' to 'PVOID' of greater size
#pragma warning(suppress:4306)
    return (PVOID)(CURRENT_CPU_MASK | CpuGetApicId());
}

#endif // _CPU_H_