#ifndef _EPT_H_
#define _EPT_H_

#include "minihv.h"

#define SHIFT_FOR_EPT_PHYSICAL_ADDR         PAGE_SHIFT

#define MASK_EPT_PML4_OFFSET(gpa)           (((QWORD)(gpa)>>39)&0x1FF)
#define MASK_EPT_PDPTE_OFFSET(gpa)          (((QWORD)(gpa)>>30)&0x1FF)
#define MASK_EPT_PDE_OFFSET(gpa)            (((QWORD)(gpa)>>21)&0x1FF)
#define MASK_EPT_PTE_OFFSET(gpa)            (((QWORD)(gpa)>>12)&0x1FF)
#define MASK_EPT_PAGE_OFFSET(gpa)           ((QWORD)(gpa)&0xFFF)

// these seem useless now but may be used in the future :)
#define GPA2HPA(gpa)                        ((QWORD)gpa)
#define HPA2GPA(hpa)                        ((QWORD)hpa)

#define EPT_READ_ACCESS                     ((QWORD)1<<0)
#define EPT_WRITE_ACCESS                    ((QWORD)1<<1)
#define EPT_EXEC_ACCESS                     ((QWORD)1<<2)
#define EPT_ALL_ACCESS                      (EPT_READ_ACCESS|EPT_WRITE_ACCESS|EPT_EXEC_ACCESS)

#define EPT_INVALIDATE_CURRENT              1
#define EPT_INVALIDATE_ALL                  2

#pragma pack(push,1)

#pragma warning(push)

//warning C4214: nonstandard extension used : bit field types other than int
#pragma warning(disable:4214)

typedef struct _EPTP
{
    // 2:0
    // 0 = Uncacheable (UC)
    // 6 = Write-Back (WB)
    QWORD       MemoryType                  :   3;

    // 5:3
    // This value must be EPT page-walk length - 1
    QWORD       PageWalkLength              :   3;

    // 6
    // If set => accessed and dirty bits will be set
    QWORD       ActivateAccessedAndDirty    :   1;

    // 11:7
    QWORD       Reserved0                   :   5;

    // (MAX_PHYS-1):12
    QWORD       PhysicalAddress             :   MAXPHYADDR-12;

    // 63:MAX_PHYS
    QWORD       Reserved1                   :   64-MAXPHYADDR;
} EPTP, *PEPTP;
STATIC_ASSERT( sizeof( EPTP ) == sizeof( QWORD ) );

// A EPT_PML4_ENTRY points to a EPT_PDPT
typedef struct _EPT_PML4_ENTRY
{
    // 0
    QWORD       Read                        :   1;

    // 1
    QWORD       Write                       :   1;

    // 2
    QWORD       Execute                     :   1;

    // 7:3
    QWORD       Reserved0                   :   5;

    // 8
    QWORD       Accessed                    :   1;

    // 11:9
    QWORD       Ignored0                    :   3;

    // (MAX_PHYS-1):12
    QWORD       PhysicalAddress             :   MAXPHYADDR-12;

    // 63:MAX_PHYS
    QWORD       Ignored1                    :   64-MAXPHYADDR;

} EPT_PML4_ENTRY, *PEPT_PML4_ENTRY;
STATIC_ASSERT( sizeof( EPT_PML4_ENTRY ) == sizeof( QWORD ) );

// A EPT_PDPT_ENTRY_PD points to a EPT_PD
typedef struct _EPT_PDPT_ENTRY_PD
{
    // 0
    QWORD       Read                        :   1;

    // 1
    QWORD       Write                       :   1;

    // 2
    QWORD       Execute                     :   1;

    // 7:3
    QWORD       Reserved0                   :   5;

    // 8
    QWORD       Accessed                    :   1;

    // 11:9
    QWORD       Ignored0                    :   3;

    // (MAX_PHYS-1):12
    QWORD       PhysicalAddress             :   MAXPHYADDR-12;

    // 63:MAX_PHYS
    QWORD       Ignored1                    :   64-MAXPHYADDR;

} EPT_PDPT_ENTRY_PD, *PEPT_PDPT_ENTRY_PD;
STATIC_ASSERT( sizeof( EPT_PDPT_ENTRY_PD ) == sizeof( QWORD ) );

// A EPT_PDPT_ENTRY_1G maps a 1-GByte Page
typedef struct _EPT_PDPT_ENTRY_1G
{
    // 0
    QWORD       Read                        :   1;

    // 1
    QWORD       Write                       :   1;

    // 2
    QWORD       Execute                     :   1;

    // 5:3
    QWORD       MemoryType                  :   3;

    // 6
    QWORD       IgnorePAT                   :   1;

    // 7
    QWORD       PageSize                    :   1;  // must be 1, else points to EPT_PD

    // 8
    QWORD       Accessed                    :   1;

    // 9
    QWORD       Dirty                       :   1;

    // 11:10
    QWORD       Ignored0                    :   2;

    // 29:12
    QWORD       Reserved0                   :   18;

    // (MAX_PHYS-1):12
    QWORD       PhysicalAddress             :   MAXPHYADDR-30;

    // 62:52
    QWORD       Ignored1                    :   11;

    // 63
    QWORD       SupressVE                   :   1;

} EPT_PDPT_ENTRY_1G, *PEPT_PDPT_ENTRY_1G;
STATIC_ASSERT( sizeof( EPT_PDPT_ENTRY_1G ) == sizeof( QWORD ) );

// A EPT_PD_ENTRY_PT points to a EPT_PT
typedef struct _EPT_PD_ENTRY_PT
{
    // 0
    QWORD       Read                        :   1;

    // 1
    QWORD       Write                       :   1;

    // 2
    QWORD       Execute                     :   1;

    // 7:3
    QWORD       Reserved0                   :   5;

    // 8
    QWORD       Accessed                    :   1;

    // 11:9
    QWORD       Ignored0                    :   3;

    // (MAX_PHYS-1):12
    QWORD       PhysicalAddress             :   MAXPHYADDR-12;

    // 63:MAX_PHYS
    QWORD       Ignored1                    :   64-MAXPHYADDR;

} EPT_PD_ENTRY_PT, *PEPT_PD_ENTRY_PT;
STATIC_ASSERT( sizeof( EPT_PD_ENTRY_PT ) == sizeof( QWORD ) );

// A EPT_PD_ENTRY_2MB maps a 2-MByte Page
typedef struct _EPT_PD_ENTRY_2MB
{
    // 0
    QWORD       Read                        :   1;

    // 1
    QWORD       Write                       :   1;

    // 2
    QWORD       Execute                     :   1;

    // 5:3
    QWORD       MemoryType                  :   3;

    // 6
    QWORD       IgnorePAT                   :   1;

    // 7
    QWORD       PageSize                    :   1;  // must be 1, else points to EPT_PD

    // 8
    QWORD       Accessed                    :   1;

    // 9
    QWORD       Dirty                       :   1;

    // 11:10
    QWORD       Ignored0                    :   2;

    // 20:12
    QWORD       Reserved0                   :   9;

    // (MAX_PHYS-1):12
    QWORD       PhysicalAddress             :   MAXPHYADDR-21;

    // 62:52
    QWORD       Ignored1                    :   11;

    // 63
    QWORD       SupressVE                   :   1;

} EPT_PD_ENTRY_2MB, *PEPT_PD_ENTRY_2MB;
STATIC_ASSERT( sizeof( EPT_PD_ENTRY_2MB ) == sizeof( QWORD ) );

// A EPT_PT_ENTRY maps a 4-KByte page
typedef struct _EPT_PT_ENTRY
{
    // 0
    QWORD       Read                        :   1;

    // 1
    QWORD       Write                       :   1;

    // 2
    QWORD       Execute                     :   1;

    // 5:3
    QWORD       MemoryType                  :   3;

    // 6
    QWORD       IgnorePAT                   :   1;

    // 7
    QWORD       Ignored0                    :   1;

    // 8
    QWORD       Accessed                    :   1;

    // 9
    QWORD       Dirty                       :   1;

    // 11:10
    QWORD       Ignored1                    :   2;

    // (MAX_PHYS-1):12
    QWORD       PhysicalAddress             :   MAXPHYADDR-12;

    // 62:52
    QWORD       Ignored2                    :   11;

    // 63
    QWORD       SupressVE                   :   1;

} EPT_PT_ENTRY, *PEPT_PT_ENTRY;
STATIC_ASSERT( sizeof( EPT_PT_ENTRY ) == sizeof( QWORD ) );
#pragma warning(pop)
#pragma pack(pop)

// Present = Read | Write | Execute
#define IsEptEntryPresent(Ept)              (0 != ((*(BYTE*)Ept)&EPT_ALL_ACCESS))

//******************************************************************************
// Function:      ConfigureEPTP
// Description: Sets up EPT pointer.
// Returns:       STATUS
// Parameter:     OUT EPTP* Ept - EPT pointer
//******************************************************************************
STATUS
ConfigureEPTP(
    OUT     EPTP*       Ept
    );

//******************************************************************************
// Function:      EptMapGuestPA
// Description: Maps a guest physical address(GPA) to a host physical address
//              (HPA).
// Returns:       PVOID - Mapped address
// Parameter:     IN PVOID GuestPA - Address to Map
// Parameter:     IN DWORD Size - Size of mapping
//******************************************************************************
PVOID
EptMapGuestPA(
    IN      PVOID       GuestPA,
    IN      DWORD       Size,
    IN      BYTE        MemoryType,
    IN_OPT  PVOID       HostPA,
    IN      BYTE        RwxAccess,
    IN      BOOLEAN     Overwrite,
    IN      BOOLEAN     Invalidate
    );

STATUS
EptInvalidate(
    IN      QWORD       Type,
    IN_OPT  EPTP*       Ept
);

typedef
VMX_RESULT
( __cdecl *INVALIDATE_EPT )(
    IN QWORD Type,
    IN PVOID Descriptor
    );

extern INVALIDATE_EPT __inv_ept;

#endif // _EPT_H_