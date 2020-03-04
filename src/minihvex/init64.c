#include "minihv.h"
#include "native/memory.h"
#include "multiboot.h"
#include "display.h"
#include "dmp_multiboot.h"
#include "dmp_apic.h"
#include "dmp_cpu.h"
#include "dmp_memory.h"
#include "dmp_minihv.h"
#include "serial.h"
#include "log.h"
#include "list.h"
#include "check_input.h"
#include "hv_heap.h"
#include "acpi_interface.h"
#include "idt.h"
#include "idt_handlers.h"
#include "apic.h"
#include "init_asm.h"
#include "pit.h"
#include "pte.h"
#include "paging_tables.h"
#include "segment.h"
#include "vmx_operations.h"
#include "vmcs.h"
#include "task.h"
#include "vmx_capability.h"
#include "vmguest.h"
#include "dmp_mtrr.h"
#include "ept.h"
#include "pci.h"
#include "data.h"
#include "dmp_common.h"

GLOBAL_DATA             gGlobalData;

extern void __gdtr();

int _fltused = 1;

void
PreInitGlobalVariables(
    void
    )
{
    // clear video display
    DispPreinitScreen(0, LINES_PER_SCREEN);
    memzero( &gGlobalData, sizeof( GLOBAL_DATA ) );

    InitializeListHead( &gGlobalData.ApicData.PhysCpuListHead );
    InitializeListHead( &gGlobalData.SystemInformation.MemoryMap.MemoryMap );

    ImportAsmFunctions();
    CpuMuCollectFeatures();
    DumpPreinit();

#pragma warning(suppress:4054)
    gGlobalData.Gdt = ( GDT* ) __gdtr;

    _InterlockedAnd( &( gGlobalData.VmxCurrentSettings.CpusInVMXMode ), 0x0 );
}

void
InitGlobalVariables(
    IN ASM_PARAMETERS*  Parameters
    )
{
    STATUS status;
    QWORD heapSize;

    LockInit(&gGlobalData.LogData.DisplayLock);
    LockInit(&gGlobalData.LogData.SerialLock);
    LockInit(&gGlobalData.PagingData.PagingLock);

    // we enable what features we can
    CpuMuEnableFeatures();

    status = Int15DetermineMemoryMapParameters( (INT15_MEMORY_MAP_ENTRY*) PA2VA(Parameters->MemoryMapAddress),
                                                Parameters->MemoryMapEntries,
                                                &(gGlobalData.SystemInformation.AvailableSystemMemory),
                                                &(gGlobalData.SystemInformation.HighestAvailablePhysicalAddress)
                                                );
    ASSERT(SUCCEEDED(status));

    heapSize = MINIHV_ADDITIONAL_SIZE + AlignAddressUpper( gGlobalData.SystemInformation.AvailableSystemMemory / 100, MB_SIZE );

    gGlobalData.MiniHvInformation.KernelBase = Parameters->KernelPhysicalBaseAddress;
    gGlobalData.MiniHvInformation.CodeLength = Parameters->KernelSize;
    gGlobalData.MiniHvInformation.DataLength = heapSize;
    gGlobalData.MiniHvInformation.TotalLength = gGlobalData.MiniHvInformation.CodeLength + gGlobalData.MiniHvInformation.DataLength;
    gGlobalData.MiniHvInformation.InitialStackBase = (PVOID) ( PA2VA(Parameters->KernelPhysicalBaseAddress) + OFFSET_TO_INITIAL_STACK_BASE );
    gGlobalData.MiniHvInformation.TrampolineSipiVector = (BYTE) ( Parameters->TrampolineEntryPoint >> SHIFT_FOR_PHYSICAL_ADDR );

    gGlobalData.VmxCurrentSettings.GuestPreloaderAddress = Parameters->GuestPreloaderAddress;

    if( !IsBooleanFlagOn( Parameters->MultibootInformation->Flags, MULTIBOOT_FLAG_BOOT_DEVICES ) )
    {
        // means we weren't loaded from a BIOS disk
        gGlobalData.VmxCurrentSettings.GuestPreloaderStartDiskDrive = INITIAL_BOOT_DISK_DRIVE;
    }
    else
    {
        // we were loaded from a BIOS disk, i.e. floppy, cd/dvd or HDD
        if( INITIAL_BOOT_DISK_DRIVE != Parameters->MultibootInformation->BootDevice )
        {
            gGlobalData.VmxCurrentSettings.GuestPreloaderStartDiskDrive = INITIAL_BOOT_DISK_DRIVE;
        }
        else
        {
            gGlobalData.VmxCurrentSettings.GuestPreloaderStartDiskDrive = INITIAL_BOOT_DISK_DRIVE + 1;
        }
    }

    status = HvInitHeap(  (BYTE*) ( PA2VA(Parameters->KernelPhysicalBaseAddress) + Parameters->KernelSize ),
                                    heapSize
                                    );
    // we cannot continue without a heap
    ASSERT( SUCCEEDED( status ) );

    DispPrintString( "Heap system initialized successfully\n", BLACK_COLOR );

    if (EXPECTED_IA32_PAT_VALUES != gGlobalData.PagingData.Ia32PatValues)
    {
        LOCKLESS_LOG("[WARNING] Initial PAT32 values: 0x%X\n", gGlobalData.PagingData.Ia32PatValues);

        gGlobalData.PagingData.Ia32PatValues = EXPECTED_IA32_PAT_VALUES;
        __writemsr(IA32_PAT, gGlobalData.PagingData.Ia32PatValues);

        // force TLB flush
        __writecr3(__readcr3());
    }

    gGlobalData.PagingData.WriteBackMemoryIndex = 0;
    gGlobalData.PagingData.UncacheableMemoryIndex = 3;

    status = ApicMapRegister();
    // this should also be done with the PRE_APIC macro because it is possible
    // for the function to have failed and thus the APIC not to be initialized :)
    ASSERT( SUCCEEDED( status ) );

    DispPrintString( "ApicMapRegister finished\n", BLACK_COLOR );

#ifndef NO_COMM
    status = SerialInitialize();
    ASSERT( SUCCEEDED( status ) );

    if( STATUS_SUCCESS == status )
    {
        LOG( "Serial Initialized on port 0x%x\n", gGlobalData.SerialPortNumber );
    }
#endif

    // mapping all MiniHV data memory (code was already mapped)
    ASSERT(gGlobalData.MiniHvInformation.DataLength <= MAX_DWORD);
    ASSERT(NULL != MapMemoryInvalidate((BYTE*)gGlobalData.MiniHvInformation.KernelBase + gGlobalData.MiniHvInformation.CodeLength, (DWORD)gGlobalData.MiniHvInformation.DataLength, MEMORY_TYPE_WRITEBACK, TRUE));

    DumpMiniHvInformation(&gGlobalData.MiniHvInformation);


    LOG("Highest available memory address is: 0x%X\n", gGlobalData.SystemInformation.HighestAvailablePhysicalAddress);
    LOG("Size of available system memory: %d MB\n", gGlobalData.SystemInformation.AvailableSystemMemory / MB_SIZE);


    status = Int15NormalizeMemoryMap( (INT15_MEMORY_MAP_ENTRY*) PA2VA(Parameters->MemoryMapAddress), Parameters->MemoryMapEntries, &(gGlobalData.SystemInformation.MemoryMap));
    ASSERT(SUCCEEDED(status));

    status = PciRetrieveDevices();
    ASSERT( SUCCEEDED( status ) );

    LOG( "PhysicalAddressBits: %d\n", gGlobalData.CpuFeatures.PhysicalAddressBits );
    LOG( "PhysicalAddressMask: 0x%X\n", gGlobalData.CpuFeatures.PhysicalAddressMask );
    LOG( "Linear Address Bits: %d\n", gGlobalData.CpuFeatures.LinearAddressBits );

    LOG( "GDT base: 0x%X\n", gGlobalData.Gdt );

    // patch INT15h
    status = PatchInt15();
    ASSERT(SUCCEEDED(status));

    status = PatchMemoryMap(    &(gGlobalData.SystemInformation.MemoryMap),
                                gGlobalData.MiniHvInformation.TotalLength,
                                (QWORD)gGlobalData.MiniHvInformation.KernelBase
                                );

    ASSERT(SUCCEEDED(status));

    status = VmxConfigureGlobalStructures(&gGlobalData.VmxConfigurationData, &gGlobalData.VmxCurrentSettings);
    ASSERT(SUCCEEDED(status));

    LOG("About to initialize APIC\n");
    ApicRetrievePhysCpu(&gGlobalData.ApicData.PhysCpuListHead);

    // before we setup IDT handlers we set up the TSS
    gGlobalData.TaskData.IstUsed = IST_TO_USE;

    status = CreateNewGdt(gGlobalData.Gdt);
    ASSERT(SUCCEEDED(status));

    status = InitializeIdtHandlers(&gGlobalData.Idt);
    ASSERT(SUCCEEDED(status));

    // there is no reason why we  need MTRR's earlier
    if (gGlobalData.CpuFeatures.MtrrSupport)
    {
        status = MtrrInitializeRegions(&gGlobalData.MtrrData);
        ASSERT(SUCCEEDED(status));

        DumpMtrrData(&gGlobalData.MtrrData);

        // map all memory until MiniHV
        status = GuestMapMemoryRange(NULL, (QWORD)gGlobalData.MiniHvInformation.KernelBase);
        ASSERT(SUCCEEDED(status));

        // map all memory after MiniHV
        status = GuestMapMemoryRange((BYTE*)gGlobalData.MiniHvInformation.KernelBase + gGlobalData.MiniHvInformation.TotalLength,
                                     gGlobalData.SystemInformation.HighestAvailablePhysicalAddress );
        ASSERT(SUCCEEDED(status));

        if (gGlobalData.MiniHvInformation.RunningNested &&
            (ListSize(&gGlobalData.ApicData.PhysCpuListHead) > 1))
        {
            // On VMWARE (at least) when the L2 guest writes to the APIC ICR the VMM crashes :)
            // The solution is to hook the guest LAPIC for write access and emulate writes
            // until all the APs are woken up :)
            // We only do this if we have more than 1 CPU (besides the fact that it doesn't make sense to do this for
            // only one CPU, if we hook it we have no 'trigger' to known when to remap it for full guest access)

            //ASSERT( NULL != EptMapGuestPA( (PVOID) VA2PA(gGlobalData.ApicData.ApicBaseAddress),
            //                              PAGE_SIZE, MEMORY_TYPE_STRONG_UNCACHEABLE,
            //                              (PVOID) VA2PA(gGlobalData.ApicData.ApicBaseAddress), EPT_READ_ACCESS, TRUE, FALSE ) );
        }
    }

    // this function issues a system panic if it fails
    ApicWakeUpCpus(&gGlobalData.ApicData.PhysCpuListHead);
    LOG("Finished CPU wakeup\n")
    LOG("Number of CPUs: %d\n", ListSize(&gGlobalData.ApicData.PhysCpuListHead));

    // unmap first PML4 entry - responsible for identity mapping
    UnmapPML4Entry(0);

    // make sure all memory before MiniHV is not unmapped
    ASSERT((QWORD)gGlobalData.MiniHvInformation.KernelBase <= MAX_DWORD);
#pragma warning(suppress:4311)
    UnmapMemory((PVOID)PA2VA(0), (DWORD)gGlobalData.MiniHvInformation.KernelBase);

    // video memory can be mapped - we need to write to the screen :)
    ASSERT(NULL != MapMemory((PVOID)BASE_VIDEO_ADDRESS, VIDEO_MEMORY_SIZE));

    // we first wait for all the AP's to launch
    DWORD activeCpus = _InterlockedAnd(&(gGlobalData.ApicData.ActiveCpus), MAX_DWORD);

    // we wait for the AP's to launch
    if ( IsBooleanFlagOn( gGlobalData.VmxConfigurationData.PinBasedControls.ChosenValue, PIN_BASED_ACTIVATE_PREEMPT_TIMER ) )
    {
        while (activeCpus - 1 != (DWORD) _InterlockedAnd(&(gGlobalData.VmxCurrentSettings.CpusInVMXMode), MAX_DWORD))
        {
            // delay, if NONE the APs may not exit when the timer reaches 0
            // I have no idea why
            __inbyte(0x61);
        }
    }
    else
    {
        LOG("VMX preemption timer is not supported\n");
    }

    _InterlockedIncrement(&(gGlobalData.VmxCurrentSettings.CpusInVMXMode));

    // this is what will actually get the guest MBR started
    status = VmxStartGuest();

    ASSERT(SUCCEEDED(status));


    NOT_REACHED;
}

int Init64( int argc, PVOID argv )
{
    ASM_PARAMETERS* pParameters;
    COMMON_LIB_INIT initSettings;
    STATUS status;

    memzero(&initSettings, sizeof(COMMON_LIB_INIT));

    initSettings.Size = sizeof(COMMON_LIB_INIT);
    initSettings.AssertFunction = MiniHVAssert;
    initSettings.MonitorSupport = SUCCEEDED(CpuMuSetMonitorFilterSize(sizeof(MONITOR_LOCK)));

    status = CommonLibInit(&initSettings);
    if (!SUCCEEDED(status))
    {
        // not good lads
        __halt();
    }

    CHECK_STACK_ALIGNMENT;

    // will need to add ASSERTS here for argc and argv
    ASSERT(1 == argc);
    ASSERT(NULL != argv);

    PreInitGlobalVariables();

    pParameters = argv;

    CheckInputParameters( pParameters );

    CheckSystemState();

    InitGlobalVariables( pParameters );

    NOT_REACHED;

    return 0;
}