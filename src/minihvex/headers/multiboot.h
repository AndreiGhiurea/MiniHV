#ifndef _MULTIBOOT_H_
#define _MULTIBOOT_H_

#include "minihv.h"

#include "cpumu.h"

// Multiboot flags
#define MULTIBOOT_FLAG_MEM_AVAILABLE            (1<<0)      // flags[0]
#define MULTIBOOT_FLAG_BOOT_DEVICES             (1<<1)      // flags[1]
#define MULTIBOOT_FLAG_COMMANDLINE              (1<<2)      // flags[2]
#define MULTIBOOT_FLAG_BOOT_MODULES             (1<<3)      // flags[3]
#define MULTIBOOT_FLAG_OUT_INFO                 (1<<4)      // flags[4]
#define MULTIBOOT_FLAG_ELF_INFO                 (1<<5)      // flags[5]
#define MULTIBOOT_FLAG_MEMORY_MAP               (1<<6)      // flags[6]
#define MULTIBOOT_FLAG_DRIVERS                  (1<<7)      // flags[7]
#define MULTIBOOT_FLAG_CONFIG_TABLE             (1<<8)      // flags[8]
#define MULTIBOOT_FLAG_BOOT_LOADER_NAME         (1<<9)      // flags[9]
#define MULTIBOOT_FLAG_APM_TABLE                (1<<10)     // flags[10]
#define MULTIBOOT_FLAG_GRAPHICS_TABLE           (1<<11)     // flags[11]

/*
                +-------------------+
        -4      | size              |
                +-------------------+
        0       | base_addr         |
        8       | length            |
        16      | type              |
                +-------------------+
*/
#pragma pack(push,1)
typedef struct _MULTIBOOT_MEMORY_MAP_ENTRY
{
    DWORD       Size;
    QWORD       BaseAddress;
    QWORD       MemoryLength;
    DWORD       Type;
} MULTIBOOT_MEMORY_MAP_ENTRY, *PMULTIBOOT_MEMORY_MAP_ENTRY;


/*
             +-------------------+
     0       | flags             |    (required)
             +-------------------+
     4       | mem_lower         |    (present if flags[0] is set)
     8       | mem_upper         |    (present if flags[0] is set)
             +-------------------+
     12      | boot_device       |    (present if flags[1] is set)
             +-------------------+
     16      | cmdline           |    (present if flags[2] is set)
             +-------------------+
     20      | mods_count        |    (present if flags[3] is set)
     24      | mods_addr         |    (present if flags[3] is set)
             +-------------------+
     28 - 40 | syms              |    (present if flags[4] or
             |                   |                flags[5] is set)
             +-------------------+
     44      | mmap_length       |    (present if flags[6] is set)
     48      | mmap_addr         |    (present if flags[6] is set)
             +-------------------+
     52      | drives_length     |    (present if flags[7] is set)
     56      | drives_addr       |    (present if flags[7] is set)
             +-------------------+
     60      | config_table      |    (present if flags[8] is set)
             +-------------------+
     64      | boot_loader_name  |    (present if flags[9] is set)
             +-------------------+
     68      | apm_table         |    (present if flags[10] is set)
             +-------------------+
     72      | vbe_control_info  |    (present if flags[11] is set)
     76      | vbe_mode_info     |
     80      | vbe_mode          |
     82      | vbe_interface_seg |
     84      | vbe_interface_off |
     86      | vbe_interface_len |
             +-------------------+
             */

typedef struct _MULTIBOOT_INFORMATION
{
    // indicates the presence of the other fields
    DWORD       Flags;                          // 0x0

    // number of KB's of lower/higher memory size

    // starts at address 0
    DWORD       LowerMemorySize;                // 0x4

    // starts at address 1MB
    DWORD       HigherMemorySize;               // 0x8


    /// should be modified to a struct in the future

    // information about the boot device
    DWORD       BootDevice;                     // 0xC

    // C-style zero-terminated string
    DWORD       CommandLine;                    // 0x10

    // number of modules loaded
    DWORD       ModuleCount;                    // 0x14

    // pointer to array of module structure
    DWORD       ModuleAddress;                  // 0x18

    /// currently not interested in this field
    BYTE        Reserved[16];                   // 0x1C

    // Memory map address
    DWORD       MemoryMapSize;                  // 0x30

    // Memory map - size of buffer
    DWORD       MemoryMapAddress;               // 0x3C

    // Drive Structures start address
    DWORD       DriveStructuresAddress;         // 0x34

    // Drive Structures size
    DWORD       DriveStructuresSize;            // 0x38

    // address of ROM configuration table
    DWORD       ConfigTableAddress;             // 0x3C

    // pointer to C-style zero terminated string
    DWORD       BootLoaderName;                 // 0x40

    // pointer to APM table
    DWORD       ApmTable;                       // 0x44

    // we don't care about no graphics
    BYTE        GraphicsData[20];               // 0x48
} MULTIBOOT_INFORMATION, *PMULTIBOOT_INFORMATION;

// parameter structure
typedef struct _ASM_PARAMETERS
{
    MULTIBOOT_INFORMATION*      MultibootInformation;
    PVOID                       KernelPhysicalBaseAddress;
    QWORD                       KernelSize;
    DWORD                       TrampolineEntryPoint;
    DWORD                       MemoryMapEntries;
    PVOID                       MemoryMapAddress;
    DWORD                       GuestPreloaderAddress;
} ASM_PARAMETERS, *PASM_PARAMETERS;
STATIC_ASSERT_INFO(sizeof(ASM_PARAMETERS) < 0x400, "Because of the way __in32.yasm is layed out parameters cannot grow past 0x400");

#pragma pack(pop)

#endif // _MULTIBOOT_H_