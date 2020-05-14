#include "vmexit.h"
#include "vmcs.h"
#include "display.h"
#include "log.h"
#include "ept.h"
#include "vmguest.h"
#include "dmp_cpu.h"
#include "paging_tables.h"
#include "dmp_memory.h"
#include "vmmemory.h"
#include "idt_handlers.h"
#include "dmp_vmcs.h"
#include "native/memory.h"
#include "data.h"
#include "distorm.h"
#include "intro.h"
#include "mzpe.h"

#pragma warning(push)

// warning C28039: The type of actual parameter '24576|2048|((0<<1))|0' should exactly match the type 'VMCS_FIELD':
#pragma warning(disable: 28039)

#define NO_OF_EXITS_TO_START_LOG            ((DWORD)-1LL)

#define MAX_CPU                             32

static volatile BYTE __vector[MAX_CPU] = { 0 };
static BOOLEAN __wroteSIPI[MAX_CPU] = { 0 };

__forceinline
static
void
VmAdvanceGuestRipByInstrLength(
    INOUT   REGISTER_AREA*    ProcessorState
);

// 4
static
STATUS
VmExitSolveSIPI(
    IN          EXIT_QUALIFICATION_SIPI*    ExitQualification
    );

// 9
static
STATUS
VmExitSolveTaskSwitch(
    IN          QWORD Reason
    );

static
STATUS
_VmExitSolveLapicAccessViolation(
    IN          DWORD               LapicOffset
    );

// 18
static
STATUS
VmExitSolveVmCall(
    INOUT       REGISTER_AREA*            ProcessorState
);

// 32
static
STATUS
VmExitSolveWrmsr(
    INOUT   REGISTER_AREA* ProcesorState
    );

// 55
static
STATUS
VmExitSolveXsetbv(
    INOUT      REGISTER_AREA*    ProcessorState
);

void
VmExitHandler(
    INOUT       COMPLETE_PROCESSOR_STATE*        ProcessorState
    )
{
    QWORD guestRIP;
    QWORD exitQualification;
    QWORD rflags;
    BOOLEAN solvedProblem;
    STATUS status;
    EXIT_REASON_STRUCT exitReason;
    VCPU* exitingVcpu;
    QWORD guestRSP;

    CHECK_STACK_ALIGNMENT;

    AssertHvIsStillFunctional();

    exitQualification = 0;
    solvedProblem = FALSE;
    status = STATUS_SUCCESS;
    exitingVcpu = GetCurrentVcpu();

    ASSERT( NULL != ProcessorState );

    // set the processor state
    exitingVcpu->ProcessorState = ProcessorState;

    // we read RSP and update the processor state
    guestRSP = ProcessorState->RegisterArea.RegisterValues[RegisterRsp] = VmxRead(VMCS_GUEST_RSP);

    // we read RFLAGS and update the processor state
    rflags = ProcessorState->RegisterArea.Rflags = VmxRead(VMCS_GUEST_RFLAGS);

    // we read RIP and update the processor state
    guestRIP = ProcessorState->RegisterArea.Rip = VmxRead(VMCS_GUEST_RIP);

    exitReason.Raw = (DWORD) VmxRead(VMCS_EXIT_REASON);


    if (0 != exitReason.EntryFailure)
    {
        LOGP("Vm entry failure due to exit reason: %u\n", exitReason.BasicExitReason);

    }

    ASSERT_INFO( 0 == ( exitReason.EntryFailure ), "Exit reason: %d\n", exitReason.BasicExitReason );
    ASSERT(0 == (exitReason.RootExit));
    ASSERT(0 == (exitReason.PendingMTF));

    exitQualification = VmxRead( VMCS_EXIT_QUALIFICATION);

    switch( exitReason.BasicExitReason )
    {
    case VM_EXIT_TRIPLE_FAULT:              // 2
        LOG( "Triple fault\n" );
        break;
    case VM_EXIT_INIT_SIGNAL:               // 3
        LOGP("INIT\n");
        if( exitingVcpu->WaitingForWakeup )
        {
            // 1.
            // this branch is taken when APs are waiting to be woken up
            // (THIS happens ONLY if the processor does not support the wait-for-SIPI activity state)
            LOGP("Pseudo-SIPI\n");
            status = VmExitSolveSIPI((EXIT_QUALIFICATION_SIPI*)&exitQualification);
            if (SUCCEEDED(status))
            {
                solvedProblem = TRUE;
            }
            else
            {
                LOG("VmExitSolveSIPI failed with status: 0x%x\n", status);
            }
        }
        else
        {
            // 2.a we may either have the BSP which sends INIT signals to the APs

            // 2.b we may have a processor which received an INIT signal as a result of an
            // INVLPG or INVEPT

            solvedProblem = TRUE;
        }
        break;
    case VM_EXIT_SIPI_SIGNAL:               // 4
        LOGP("SIPI\n");
        status = VmExitSolveSIPI( ( EXIT_QUALIFICATION_SIPI* ) &exitQualification );
        if( SUCCEEDED( status ) )
        {
            solvedProblem = TRUE;
        }
        else
        {
            LOG( "VmExitSolveSIPI failed with status: 0x%x\n", status );
        }
        break;
    case VM_EXIT_TASK_SWITCH:               // 9
        LOG("TASK SWITCH EXIT\n");
        status = VmExitSolveTaskSwitch(exitQualification);
        if (SUCCEEDED(status))
        {
            solvedProblem = TRUE;
        }
        else
        {
            LOG("VmExitSolveTaskSwitch failed with status 0x%x\n", status);
        }
        break;
    case VM_EXIT_VMCALL:                    // 18
        status = VmExitSolveVmCall(&ProcessorState->RegisterArea);
        if (SUCCEEDED(status))
        {
            solvedProblem = TRUE;
        }
        else
        {
            LOG("VmExitSolveVmCall failed with status: 0x%x\n", status);
        }
        break;
    case VM_EXIT_CPUID:
    {
        DWORD index = (DWORD)ProcessorState->RegisterArea.RegisterValues[RegisterRax];
        DWORD subIndex = (DWORD)ProcessorState->RegisterArea.RegisterValues[RegisterRcx];

        CPUID_INFO cpuInfo = { 0 };

        __cpuidex(cpuInfo.values, index, subIndex);

        // A value of 1 indicates that the OS has set CR4.OSXSAVE[bit 18] to enable XSETBV / XGETBV
        // instructions to access XCR0 and to support processor extended state management using
        // XSAVE / XRSTOR

        if (index == 1)
        {
            cpuInfo.FeatureInformation.ecx.VMX = 0;
            cpuInfo.FeatureInformation.ecx.SMX = 0;
            cpuInfo.FeatureInformation.ecx.HV = 0;

            cpuInfo.FeatureInformation.ecx.OSXSAVE = IsBooleanFlagOn(VmxRead(VMCS_GUEST_CR4), CR4_OSXSAVE);
        }

        ProcessorState->RegisterArea.RegisterValues[RegisterRax] = cpuInfo.eax;
        ProcessorState->RegisterArea.RegisterValues[RegisterRbx] = cpuInfo.ebx;
        ProcessorState->RegisterArea.RegisterValues[RegisterRcx] = cpuInfo.ecx;
        ProcessorState->RegisterArea.RegisterValues[RegisterRdx] = cpuInfo.edx;



        VmAdvanceGuestRipByInstrLength(&ProcessorState->RegisterArea);

        solvedProblem = TRUE;


    }
    break;
    case VM_EXIT_WRMSR:                 // 32
        status = VmExitSolveWrmsr(&ProcessorState->RegisterArea);
        if (SUCCEEDED(status))
        {
            solvedProblem = TRUE;

            // Initialize intro here
            if (0 != gGlobalData.Intro.SyscallEip)
            {
                status = IntFindKernelBase(gGlobalData.Intro.SyscallEip);
            }

            if (!SUCCEEDED(status))
            {
                LOGL("IntFindKernelBase failed with status: 0x%x\n", status);
            }
        }
        else
        {
            LOG("VmExitSolveWrmsr failed with status: 0x%x\n", status);
        }
        break;
    case VM_EXIT_XSETBV:                // 55
        status = VmExitSolveXsetbv(&ProcessorState->RegisterArea);
        if (SUCCEEDED(status))
        {
            solvedProblem = TRUE;
        }
        else
        {
            LOG("VmExitSolveXsetbv failed with status: 0x%x\n", status);
        }
        break;
    default:
        LOG( "Exit reason unknown: %d\n", exitReason.BasicExitReason );
    }

    if( solvedProblem )
    {

        if (guestRIP != ProcessorState->RegisterArea.Rip)
        {
            QWORD intState;
            QWORD newIntState;

            // we need to update rip
            VmxWrite(VMCS_GUEST_RIP, ProcessorState->RegisterArea.Rip);

            // we clear RF
            ProcessorState->RegisterArea.Rflags = ProcessorState->RegisterArea.Rflags & (~( RFLAGS_RESUME_FLAG_BIT ) );

            if( rflags != ProcessorState->RegisterArea.Rflags )
            {
                VmxWrite(VMCS_GUEST_RFLAGS, ProcessorState->RegisterArea.Rflags);
            }

            // if we modified the RIP => we 'executed' an instruction => we need to clear Blocking by STI and by MOV SS
            intState = VmxRead(VMCS_GUEST_INT_STATE);

            newIntState = intState & ( ~( INT_STATE_BLOCKING_BY_STI | INT_STATE_BLOCKING_BY_MOV_SS ) );

            if( newIntState != intState )
            {
                VmxWrite(VMCS_GUEST_INT_STATE, newIntState);
                //LOG( "Interrupt state: 0x%X [0x%X]\n", newIntState, intState );
            }
        }

        if (guestRSP != ProcessorState->RegisterArea.RegisterValues[RegisterRsp])
        {
            VmxWrite(VMCS_GUEST_RSP, ProcessorState->RegisterArea.RegisterValues[RegisterRsp]);
        }

        VmxResumeGuest();
    }
    else
    {
        DumpCurrentVmcs(&ProcessorState->RegisterArea);

        ASSERT_INFO(FALSE, "We couldn't solve VM Exit %d\n", exitReason.BasicExitReason);
    }
}

__forceinline
static
void
VmAdvanceGuestRipByInstrLength(
    INOUT   REGISTER_AREA*    ProcessorState
)
{
    ProcessorState->Rip += VmxRead(VMCS_EXIT_INSTRUCTION_LENGTH);
}

// 4
static
STATUS
VmExitSolveSIPI(
    IN      EXIT_QUALIFICATION_SIPI* ExitQualification
    )
{
    QWORD actualCS;
    STATUS status;
    PVCPU pVcpu;

    if( NULL == ExitQualification )
    {
        return STATUS_INVALID_PARAMETER1;
    }

    pVcpu = GetCurrentVcpu();

    if (!pVcpu->ReceivedSIPI)
    {
        LOGP("Exit qualification: 0x%X\n", *((QWORD*)ExitQualification));

        actualCS = gGlobalData.MiniHvInformation.RunningNested ?
            (((QWORD)__vector[CpuGetApicId()]) << 8 ) :
            ( ExitQualification->Vector << 8);

        VmxWrite(VMCS_GUEST_RIP, 0);

        VmxWrite(VMCS_GUEST_CS_SELECTOR, actualCS);

        VmxWrite(VMCS_GUEST_CS_BASE, actualCS << 4);

        GetCurrentVcpu()->ProcessorState->RegisterArea.RegisterValues[RegisterRdx] = gGlobalData.VmxCurrentSettings.GuestPreloaderStartDiskDrive;

    status = VmxSetActivityState(ActivityStateActive);
    ASSERT(SUCCEEDED(status));

        pVcpu->WaitingForWakeup = FALSE;
        pVcpu->ExpectingGPAfterSIPI = TRUE;

        pVcpu->ReceivedSIPI = TRUE;
    }

    return STATUS_SUCCESS;
}

// 9
static
STATUS
VmExitSolveTaskSwitch(
    IN QWORD Reason
    )
{
    QWORD reason;

    reason = (Reason >> 30) & 0x3;
    switch (reason) {
    case 0:
        LOG("TSR CALL\n");
    case 1:
        LOG("TSR_IRET\n");
    case 2:
        LOG("TSR_JMP\n");
    case 3:
        LOG("TSR_IDT_GATE\n");
    default:
        LOG("Invalid reason %d\n", reason);
    }

    return STATUS_SUCCESS;
}

// 32
static
STATUS
VmExitSolveWrmsr(
    INOUT   REGISTER_AREA* ProcessorState)
{
    DWORD msrIndex;
    QWORD msrValue;
    DWORD errorCode = 0;
    STATUS status;

    msrIndex = (DWORD)ProcessorState->RegisterValues[RegisterRcx];
    status = STATUS_SUCCESS;
    msrValue = DWORDS_TO_QWORD(ProcessorState->RegisterValues[RegisterRdx], ProcessorState->RegisterValues[RegisterRax]);

    switch (msrIndex)
    {
    case SYSENTER_EIP_MSR:
        status = STATUS_SUCCESS;
        VmxWrite(VMCS_GUEST_IA32_SYSENTER_EIP, msrValue);
        gGlobalData.Intro.SyscallEip = msrValue;
        break;
    default:
        status = STATUS_UNSUCCESSFUL;
    }
    
    if (!SUCCEEDED(status))
    {
        status = GuestInjectEvent(ExceptionGeneralProtection, InterruptionTypeHardwareException, &errorCode);
    }
    else
    {
        LOGL("WRMSR at index 0x%x with value: 0x%X\n", msrIndex, msrValue);
        VmAdvanceGuestRipByInstrLength(ProcessorState);
    }

    return STATUS_SUCCESS;
}

static
STATUS
_VmExitSolveLapicAccessViolation(
    IN          DWORD               LapicOffset
    )
{
    DWORD value = MAX_DWORD;
    _DInst instr;
    PVOID hostVa;
    _CodeInfo ci;
    DWORD noOfInstr;
    _DecodeResult res;
    PVCPU pVcpu;

    pVcpu = GetCurrentVcpu();

    ASSERT(NULL != pVcpu);
    memzero(&ci, sizeof(_CodeInfo));

    ASSERT(SUCCEEDED(GuestVAToHostVA(pVcpu->ProcessorState->RegisterArea.Rip, NULL, &hostVa)));

    ci.code = hostVa;
    ci.codeLen = 20;
    ci.dt = Decode64Bits;

    res = distorm_decompose(&ci, &instr, 1, &noOfInstr);

    ASSERT_INFO(res == DECRES_SUCCESS || res == DECRES_MEMORYERR, "res: %d\n", res);
    ASSERT(1 == noOfInstr);

    // because we allow read access to the guest it's clear that it's going to be
    // a write here => we will use operand 1 to determine the source

    ASSERT(instr.ops[1].size == BITS_FOR_STRUCTURE(DWORD));
    if (instr.ops[1].type == O_REG)
    {
        DWORD idx = instr.ops[1].index - REGS32_BASE;
        value = QWORD_LOW(pVcpu->ProcessorState->RegisterArea.RegisterValues[idx]);
    }
    else if (instr.ops[1].type == O_IMM)
    {
        value = instr.imm.dword;
    }
    else
    {
        NOT_REACHED;
    }

    if (LapicOffset == APIC_ICR_HIGH_REG_OFFSET)
    {
        ICR_HIGH_REGISTER* icr = (ICR_HIGH_REGISTER*)&value;
        pVcpu->IcrHighApicId = (BYTE)icr->Destination;
        LOGP("Icr high destination: %u\n", pVcpu->IcrHighApicId);
    }
    else if (LapicOffset == APIC_ICR_LOW_REG_OFFSET)
    {
#pragma warning(suppress:4305)
        ICR_LOW_REGISTER* icr = (ICR_LOW_REGISTER*)&value;
        BYTE vect = (BYTE)icr->Vector;

        LOG("ICR LOW will be written\n");

        if (ApicDeliveryModeSIPI == icr->DeliveryMode)
        {
            __vector[pVcpu->IcrHighApicId] = vect;
            if (ApicDestinationShorthandAllExcludingSelf == icr->DestinationShorthand)
            {
                DWORD k;

                for (k = 0; k < MAX_CPU; ++k)
                {
                    __vector[k] = vect;
                }
            }


            if (!__wroteSIPI[pVcpu->IcrHighApicId])
            {
                ApicSendIpi(pVcpu->IcrHighApicId,
                            icr->DeliveryMode, icr->DestinationShorthand, &vect);

                if (ApicDestinationShorthandAllExcludingSelf == icr->DestinationShorthand)
                {
                    DWORD k;

                    for (k = 0; k < MAX_CPU; ++k)
                    {
                        __wroteSIPI[k] = TRUE;
                    }
                }
                else
                {
                    __wroteSIPI[pVcpu->IcrHighApicId] = TRUE;
                }
            }
            else
            {
                DWORD cpusReceivedSIPI;

                LOGP("2nd SIPI\n");
                ApicSendIpi(pVcpu->IcrHighApicId,
                            ApicDeliveryModeINIT, icr->DestinationShorthand, &vect);

                if (ApicDestinationShorthandAllExcludingSelf == icr->DestinationShorthand)
                {
                    cpusReceivedSIPI = gGlobalData.ApicData.ActiveCpus - 1;
                }
                else
                {
                    cpusReceivedSIPI = _InterlockedIncrement(&gGlobalData.VmxCurrentSettings.CpusReceivedSIPI);
                }

                if (cpusReceivedSIPI == gGlobalData.ApicData.ActiveCpus - 1)
                {
                    ASSERT(NULL != EptMapGuestPA((PVOID)VA2PA(gGlobalData.ApicData.ApicBaseAddress),
                           PAGE_SIZE, MEMORY_TYPE_STRONG_UNCACHEABLE,
                           (PVOID)VA2PA(gGlobalData.ApicData.ApicBaseAddress), EPT_READ_ACCESS | EPT_WRITE_ACCESS, TRUE, FALSE));
                    LOGP("Mapped LAPIC for guest access\n");
                }
            }
        }
        else if (ApicDeliveryModeINIT != icr->DeliveryMode)
        {
            LOGP("Shorthand: %u\n", icr->DestinationShorthand);
            LOGP("Mode: %u\n", icr->DeliveryMode);
            LOGP("Vector: 0x%x\n", vect);
            LOGP("Apic ID: 0x%x\n", pVcpu->IcrHighApicId);
            ApicSendIpi(pVcpu->IcrHighApicId,
                        icr->DeliveryMode, icr->DestinationShorthand, &vect);
        }
        else
        {
            ASSERT(ApicDeliveryModeINIT == icr->DeliveryMode);
        }


    }
    else
    {
        volatile DWORD* pLapic = (volatile DWORD*) PtrOffset(gGlobalData.ApicData.ApicBaseAddress, LapicOffset);
        *pLapic = value;
    }

    pVcpu->ProcessorState->RegisterArea.Rip += instr.size;

    return STATUS_SUCCESS;
}

static
STATUS
VmExitSolveVmCall(
    INOUT       REGISTER_AREA*            ProcessorState
)
{
    STATUS status;
    QWORD rax;
    WORD* stackValues;

    status = STATUS_SUCCESS;

    rax = ProcessorState->RegisterValues[RegisterRax];
    stackValues = NULL;
    
    if ((INT15_E820 == rax))
    {
        if (ProcessorState->Rip > IVT_LIMIT)
        {
            return STATUS_VMX_UNEXPECTED_VMCALL;
        }

        status = SimulateInt15h(&(gGlobalData.SystemInformation.MemoryMap));

        stackValues = (WORD*)MapMemory((PVOID)ProcessorState->RegisterValues[RegisterRsp], 3 * sizeof(WORD));
        ASSERT(0 != stackValues);
        
        if (IsBooleanFlagOn(ProcessorState->Rflags, RFLAGS_CARRY_FLAG_BIT))
        {
            *(stackValues + 2) = (*(stackValues + 2) | RFLAGS_CARRY_FLAG_BIT);
        }
        else
        {
            *(stackValues + 2) = (WORD)(*(stackValues + 2) & (~(RFLAGS_CARRY_FLAG_BIT)));
        }
        
        status = UnmapMemory(stackValues, 3 * sizeof(WORD));
        ASSERT(SUCCEEDED(status));

        VmAdvanceGuestRipByInstrLength(ProcessorState);
    }
    else if (ProcessorState->RegisterValues[RegisterRsi] == 0xABABABAB)
    {  
        // QWORD inputBufferSize = ProcessorState->RegisterValues[RegisterRax];
        // QWORD inputBufferAddr = ProcessorState->RegisterValues[RegisterRcx];
        QWORD outputBufferSize = ProcessorState->RegisterValues[RegisterRdx];
        QWORD outputBufferAddr = ProcessorState->RegisterValues[RegisterRbx];
        QWORD contextAddr = ProcessorState->RegisterValues[RegisterRdi];
        PVOID hostOutputBufferVa = NULL;
        PVOID hostOutputBufferPa = NULL;
        PVOID hostContextVa = NULL;
        PVOID hostContextPa = NULL;

        status = GuestVAToHostVA(outputBufferAddr, &hostOutputBufferPa, &hostOutputBufferVa);
        if (!SUCCEEDED(status))
        {
            LOGL("GuestVAToHostVA failed with status: 0x%x\n", status);
            goto _exit;
        }

        if (0 != contextAddr)
        {
            status = GuestVAToHostVA(contextAddr, &hostContextPa, &hostContextVa);
            if (!SUCCEEDED(status))
            {
                LOGL("GuestVAToHostVA failed with status: 0x%x\n", status);
                goto _exit;
            }
        }

        status = IntGetActiveProcessesList((DWORD)outputBufferSize, hostOutputBufferVa, hostContextVa);
        if (!SUCCEEDED(status))
        {
            LOGL("IntGetActiveEprocess failed with status: 0x%x\n", status);
        }

        status = UnmapMemory(hostOutputBufferPa, PAGE_SIZE);
        if (!SUCCEEDED(status))
        {
            LOGL("UnmapMemory failed with status: 0x%x\n", status);
        }

        if (hostContextPa != NULL)
        {
            status = UnmapMemory(hostContextPa, PAGE_SIZE);
            if (!SUCCEEDED(status))
            {
                LOGL("UnmapMemory failed with status: 0x%x\n", status);
            }
        }

    _exit:
        VmAdvanceGuestRipByInstrLength(ProcessorState);

        status = STATUS_SUCCESS;
    }
    else
    {
        status = GuestInjectEvent(ExceptionInvalidOpcode, InterruptionTypeHardwareException, NULL);
        if (!SUCCEEDED(status))
        {
            LOGPL("GuestInjectEvent failed with status: 0x%x\n", status);
            status = STATUS_SUCCESS;
        }

    }

    return status;
}

static
STATUS
VmExitSolveXsetbv(
    INOUT      REGISTER_AREA*    ProcessorState
)
{
    STATUS status;
    QWORD value;
    DWORD index;

    if (NULL == ProcessorState)
    {
        return STATUS_INVALID_PARAMETER1;
    }

    index = QWORD_LOW(ProcessorState->RegisterValues[RegisterRcx]);

    value = DWORDS_TO_QWORD(
        QWORD_LOW(ProcessorState->RegisterValues[RegisterRdx]),
        QWORD_LOW(ProcessorState->RegisterValues[RegisterRax])
    );

    LOGP("XSETBV: 0x%X\n", value);

    status = CpumuSetFpuFeatures(value);
    if (!SUCCEEDED(status))
    {
        LOG_FUNC_ERROR("CpumuSetFpuFeatures", status);
        return status;
    }

    VmAdvanceGuestRipByInstrLength(ProcessorState);

    return STATUS_SUCCESS;
}
#pragma warning(pop)
