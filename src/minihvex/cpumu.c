#include "cpumu.h"
#include "native/memory.h"
#include "data.h"

typedef struct _CPUMU_DATA
{
    CPUID_FEATURE_INFORMATION                   FeatureInformation;
    CPUID_MONITOR_LEAF                          MonitorLeaf;
    CPUID_EXTENDED_CPUID_INFORMATION            ExtendedInformation;
    CPUID_EXTENDED_FEATURE_INFORMATION          ExtendedFeatures;
    CPUID_PROCESSOR_ADDRESS_SIZES_INFORMATION   AddressSizes;
} CPUMU_DATA, *PCPMU_DATA;

static CPUMU_DATA m_cpuMuData;

void
CpuMuCollectFeatures(
    void
    )
{
    memzero(&m_cpuMuData, sizeof(CPUMU_DATA));

    __cpuid((int*)&m_cpuMuData.FeatureInformation, CpuidIdxFeatureInformation);

    __cpuid((int*)&m_cpuMuData.MonitorLeaf, CpuidIdxMonitorLeaf);

    __cpuid((int*)&m_cpuMuData.ExtendedInformation, CpuidIdxExtendedMaxFunction);

    __cpuid((int*)&m_cpuMuData.ExtendedFeatures, CpuidIdxExtendedFeatureInformation);

    __cpuid((int*)&m_cpuMuData.AddressSizes, CpuidIdxProcessorAddressSizes);
}

void
CpuMuCheckFeatures(
    void
    )
{
    // there are some essential features we need for our
    // hypervisor to work

    ASSERT(CpuIsIntel());

    // we need VMX (Virtual Machine Extensions)
    ASSERT_INFO(m_cpuMuData.FeatureInformation.ecx.VMX, "Sorry, cannot boot without VT-x support!");
    gGlobalData.CpuFeatures.XSaveSupport = (BOOLEAN) m_cpuMuData.FeatureInformation.ecx.XSAVE;
    gGlobalData.CpuFeatures.X2APICAvailable = (BOOLEAN) m_cpuMuData.FeatureInformation.ecx.x2APIC;

    ASSERT_INFO(m_cpuMuData.FeatureInformation.edx.APIC, "We cannot wake up APs without APIC!");

    ASSERT_INFO(m_cpuMuData.FeatureInformation.edx.MSR, "We cannot do anything without MSRs!");

    ASSERT_INFO(m_cpuMuData.FeatureInformation.edx.TSC, "We need a way to measure time!");

    ASSERT_INFO(m_cpuMuData.FeatureInformation.edx.PAT, "We need PAT!");

    // it would be best if we had MTRR support
    gGlobalData.CpuFeatures.MtrrSupport = (BOOLEAN) m_cpuMuData.FeatureInformation.edx.MTRR;

    // if the value is smaller then CPUID_EXTENDED_FEATURE_INFO
    // we certainly don't have support for x64
    ASSERT( m_cpuMuData.ExtendedInformation.MaxValueForExtendedInfo >= CpuidIdxProcessorAddressSizes );

    ASSERT(m_cpuMuData.ExtendedFeatures.edx.Intel64);

    gGlobalData.CpuFeatures.PhysicalAddressBits = m_cpuMuData.AddressSizes.eax.PhysicalAddressBits;
    gGlobalData.CpuFeatures.LinearAddressBits = m_cpuMuData.AddressSizes.eax.LinearAddressBits;

    // we create the mask
    gGlobalData.CpuFeatures.PhysicalAddressMask = (1ULL << (QWORD)gGlobalData.CpuFeatures.PhysicalAddressBits) - 1;

    // check if we're running nested (inside a hypervisor)
    // only works if the hypervisor provides guest enlightenment
    gGlobalData.MiniHvInformation.RunningNested = (BOOLEAN) m_cpuMuData.FeatureInformation.ecx.HV;

    gGlobalData.PagingData.Ia32PatValues = __readmsr(IA32_PAT);
}

void
CpuMuEnableFeatures(
    void
    )
{
    QWORD cr4;

    cr4 = __readcr4();

    // we need to enable VMX
    cr4 = cr4 | CR4_VMXE;

    if( gGlobalData.CpuFeatures.XSaveSupport )
    {
        // if this feature exists on the processor the guest will use it
        cr4 = cr4 | CR4_OSXSAVE;
    }

    // update CR4
    __writecr4(cr4);
}

STATUS
CpuMuSetMonitorFilterSize(
    IN          WORD        FilterSize
    )
{
    if (!m_cpuMuData.FeatureInformation.ecx.MONITOR)
    {
        // no monitor available
        return STATUS_CPU_MONITOR_NOT_SUPPORTED;
    }

    if (FilterSize < m_cpuMuData.MonitorLeaf.eax.SmallestMonitorLineSize)
    {
        return STATUS_CPU_MONITOR_FILTER_SIZE_TOO_SMALL;
    }

    if (FilterSize > m_cpuMuData.MonitorLeaf.ebx.LargestMonitorLineSize)
    {
        return STATUS_CPU_MONITOR_FILTER_SIZE_TOO_LARGE;
    }
    
    __writemsr( IA32_MONITOR_FILTER_SIZE_MSR, FilterSize);

    return STATUS_SUCCESS;
}

STATUS
CpuCreateTSSDescriptor(
    void
    )
{
    STATUS status;
    PCPU* pCpu;
    WORD noOfExistentTasks;
    WORD selectorIndex;

    pCpu = GetCurrentPcpu();
    status = STATUS_SUCCESS;
    noOfExistentTasks = 0;
    selectorIndex = 0;

    // we subtract by 1 because the _InterlockedIncrement function returns the incremented value
    noOfExistentTasks = _InterlockedIncrement16(&gGlobalData.TaskData.NoOfExistantTasks) - 1;

    // NULL + DATA + CODE
    selectorIndex = 3 * sizeof(SEGMENT_DESCRIPTOR) + noOfExistentTasks * sizeof(TSS_DESCRIPTOR);

    // we don't want to exceed GDT limit
    ASSERT((selectorIndex + sizeof(TSS_DESCRIPTOR) - 1) <= gGlobalData.Gdt->Limit);

    status = CreateNewTSSDescriptor(selectorIndex, &pCpu->TssAddress);
    ASSERT(SUCCEEDED(status));

    pCpu->TrSelector = selectorIndex;

    return status;
}

STATUS
CpumuSetFpuFeatures(
    _In_        XCR0_SAVED_STATE        Features
    )
{
    STATUS status;

    status = HalSetActiveFpuFeatures(Features);
    if (!SUCCEEDED(status))
    {
        LOG_FUNC_ERROR("HalSetActiveFpuFeatures", status);
        return status;
    }

    return status;
}
