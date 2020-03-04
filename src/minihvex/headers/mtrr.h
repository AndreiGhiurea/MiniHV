#ifndef _MTRR_H_
#define _MTRR_H_

#include "minihv.h"

#define MEMORY_ENTRIES_PER_FIXED_REGISTER           8

#pragma pack(push,1)

#pragma warning(push)

//warning C4214: nonstandard extension used : bit field types other than int
#pragma warning(disable:4214)

typedef struct _MTRR_CAPABILITY
{
    QWORD       NumberOfVariableLengthRegisters     :   8;
    QWORD       FixedRegistersSupport               :   1;
    QWORD       Reserved0                           :   1;
    QWORD       WriteCombineSupport                 :   1;
    QWORD       SMRRSupport                         :   1;
    QWORD       Reserved1                           :  52;
} MTRR_CAPABILITY, *PMTRR_CAPABILITY;
STATIC_ASSERT( sizeof( MTRR_CAPABILITY ) == sizeof( QWORD ) );
#pragma warning(pop)
#pragma pack(pop)

typedef struct _MTRR_ENTRY
{
    QWORD               BaseAddress;
    QWORD               EndAddress;

    BYTE                MemoryType;

    LIST_ENTRY          ListEntry;
} MTRR_ENTRY, *PMTRR_ENTRY;


typedef struct _MTRR_DATA
{
    MTRR_CAPABILITY     Capabilities;

    // Memory type used for memory regions outside MTRR
    BYTE                DefaultMemoryType;

    DWORD               NumberOfListEntries;
    LIST_ENTRY          MtrrRegions[MEMORY_TYPE_MAXIMUM_MTRR];
} MTRR_DATA, *PMTRR_DATA;

//******************************************************************************
// Function:    MtrrInitializeRegions
// Description: Reads the MTRR registers and sets up the structure holding
//              the MTRR map. (this is a compressed version)
// Returns:       STATUS
// Parameter:     OUT MTRR_DATA* MtrrData
// NOTE:        THIS FUNCTION MUST BE CALLED BEFORE ANY OTHER MTRR* FUNCTIONS.
//******************************************************************************
STATUS
MtrrInitializeRegions(
    OUT     MTRR_DATA*      MtrrData
    );

//******************************************************************************
// Function:    MtrrFindPhysicalAddressMemoryType
// Description: Determines the memory type of a physical memory address.
// Returns:       STATUS
// Parameter:     IN PVOID PhysicalAddress
// Parameter:     OUT BYTE * MemoryType
//******************************************************************************
STATUS
MtrrFindPhysicalAddressMemoryType(
    IN      PVOID           PhysicalAddress,
    OUT     BYTE*           MemoryType
    );

#endif // _MTRR_H_