#ifndef _SEGMENT_H_
#define _SEGMENT_H_

#include "minihv.h"

#define PREDEFINED_GDT_SIZE                     10
#define PREDEFINED_TSS_DESC_SIZE                16

// Segment Descriptor Type related information
#define LDT_SEGMENT                             2
#define DATA_SEGMENT_READ_WRITE_ACCESSED        3
#define TSS_SEGMENT_16_BIT_BUSY                 3

#define CODE_SEGMENT_EXECUTE_ONLY_ACCESSED      9
#define CODE_SEGMENT_EXECUTE_READ_ACCESSED      11
#define TSS_SEGMENT_32_BIT_BUSY                 11
#define TSS_SEGMENT_64_BIT_BUSY                 11

// Privillege levels
#define RING_ZERO_PL                0
#define NO_OF_PRIVILLEGE_LEVELS     3

// type of Segment Descriptor Types
#define TSS_AVAILABLE_ENTRY64       0x9
#define TSS_BUSY_ENTRY64            0xB
#define TASK_GATE_ENTRY64           0xC   // I hope it's the same as Call Gate
#define INTERRUPT_GATE_ENTRY64      0xE
#define TRAP_GATE_ENTRY64           0xF


#define SEGMENT_FS_SEGMENT          0x1
#define SEGMENT_GS_SEGMENT          0x2

#pragma pack(push,1)

#pragma warning(push)

//warning C4214: nonstandard extension used : bit field types other than int
#pragma warning(disable:4214)
typedef struct _SEGMENT_DESCRIPTOR
{
    // 15:0
    WORD                SegmentLimitLow;            // Segment Limit 15:0

    // 31:16
    WORD                BaseAddressLow;             // Base Address 15:0

    // 39:32
    BYTE                BaseAddressMid;             // Base Address 23:16

    // 43:40
    BYTE                Type                :4;

    // 44
    BYTE                DescriptorType      :1;     // (0 = System; 1 = code or data)

    // 46:45
    BYTE                DPL                 :2;

    // 47
    BYTE                Present             :1;

    // 51:48
    BYTE                SegmentLimitHigh    :4;     // Segment Limit 19:16

    // 52
    BYTE                AVL                 :1;     // Available for use by system SW

    // 53
    BYTE                L                   :1;     // 1 = 64-bit code segment

    // 54
    BYTE                D_B                 :1;     // Default Operation Size

    // 55
    BYTE                G                   :1;     // granularity

    // 63:56
    BYTE                BaseAddressHigh;            // Base Address 31:24

} SEGMENT_DESCRIPTOR, *PSEGMENT_DESCRIPTOR;
STATIC_ASSERT( sizeof( SEGMENT_DESCRIPTOR ) == sizeof( QWORD ) );

typedef struct _GDT
{
    // IDTR(Limit) <-- SRC[0:15];
    WORD                    Limit;

    // IDTR(Base)  <-- SRC[16:79];
    SEGMENT_DESCRIPTOR*     Base;
} GDT, *PGDT;
STATIC_ASSERT( sizeof( GDT ) == PREDEFINED_GDT_SIZE );
#pragma warning(pop)
#pragma pack(pop)

//******************************************************************************
// Function:      CreateNewGdt
// Description: Allocates a PAGE_SIZE entry for a new GDT. Sets the GDTR to
//              point to the newly allocated entry and invalidates the page
//              entries.
// Returns:       STATUS
// Parameter:     OUT GDT * NewGdt - GDTR to be modified.
//******************************************************************************
STATUS
CreateNewGdt(
    OUT     GDT*        NewGdt
    );

__forceinline
BOOLEAN
IsSegmentPrivileged(
    IN          WORD            Selector
    )
{
    return !(IsFlagOn(Selector, 0b11));
}

//******************************************************************************
// Function:      __loadGDT
// Description: Loads new GDT and sets the CS, DS(SS/FS/GS) and TR selectors.
// Returns:       void
// Parameter:     IN GDT* NewGdt
// Parameter:     IN WORD CsSelector
// Parameter:     IN WORD DsSelector
// Parameter:     IN WORD TrSelector
//******************************************************************************
typedef
void
(__cdecl* LOAD_GDT ) (
    IN      GDT*        NewGdt,
    IN      WORD        CsSelector,
    IN      WORD        DsSelector
    );

//******************************************************************************
// Function:      __loadTR
// Description: Loads a new TR selector.
// Returns:       void
// Parameter:     IN WORD Selector
//******************************************************************************
typedef
void
(__cdecl* LOAD_TR ) (
    IN       WORD       Selector
    );

extern LOAD_GDT __loadGDT;
extern LOAD_TR __loadTR;

#endif // _SEGMENT_H_