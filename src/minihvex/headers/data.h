#ifndef _DATA_H_
#define _DATA_H_


#include "base.h"
#include "cpumu.h"
#include "apic.h"
#include "idt.h"
#include "pte.h"
#include "segment.h"
#include "vmx_capability.h"
#include "lock.h"
#include "log.h"
#include "task.h"
#include "vmmemory.h"
#include "mtrr.h"
#include "paging_tables.h"

// Flags corresponding to MINIHV_INFORMATION.MiniHvFlags
#define MINIHV_FLAGS_HALTED                 ((DWORD)1<<1)

typedef struct _MINIHV_INFORMATION
{
    PVOID                       KernelBase;

    QWORD                       CodeLength;
    QWORD                       DataLength;
    QWORD                       TotalLength;

    BYTE                        TrampolineSipiVector;

    PVOID                       InitialStackBase;

    // if set to TRUE it means the hypervisor is running nested
    BOOLEAN                     RunningNested;

    volatile DWORD              MiniHvFlags;
} MINIHV_INFORMATION, *PMINIHV_INFORMATION;

typedef struct _SYSTEM_INFORMATION
{
    INT15_MEMORY_MAP            MemoryMap;

    QWORD                       AvailableSystemMemory;
    QWORD                       HighestAvailablePhysicalAddress;
} SYSTEM_INFORMATION, *PSYSTEM_INFORMATION;

typedef struct _GLOBAL_DATA
{
    MINIHV_INFORMATION          MiniHvInformation;

    SYSTEM_INFORMATION          SystemInformation;

    FEATURE_DATA                CpuFeatures;

    APIC_DATA                   ApicData;

    VMX_CONFIGURATION_DATA      VmxConfigurationData;

    VMX_SETTINGS                VmxCurrentSettings;

    LOGGING_STRUCTURE           LogData;

    WORD                        SerialPortNumber;

    IDT*                        Idt;

    PAGING_DATA                 PagingData;

    GDT*                        Gdt;

    TASK_CONFIGURATION          TaskData;

    MTRR_DATA                   MtrrData;
} GLOBAL_DATA, *PGLOBAL_DATA;

extern GLOBAL_DATA gGlobalData;

#endif // _DATA_H_