#ifndef _IDT_H_
#define _IDT_H_

#include "minihv.h"

#define PREDEFINED_IDT_SIZE                     10
#define PREDEFINED_IDT_ENTRY_SIZE               16

#pragma pack(push,1)

#pragma warning(push)

//warning C4214: nonstandard extension used : bit field types other than int
#pragma warning(disable:4214)
typedef struct _IDT_ENTRY
{
    // 15:0
    WORD            LowWordOffset;      // Bits 15:0 of address

    // 31:16
    WORD            SegmentSelector;

    // 34:32
    WORD            IST         : 3;    // IST = Interrupt Stack Table
    WORD            Reserved0   : 5;    // these must be 0 on x64
    WORD            Type        : 4;
    WORD            Reserved1   : 1;    // 0
    WORD            DPL         : 2;
    WORD            Present     : 1;
    WORD            HighWordOffset;     // Bits 31:16 of address
    DWORD           HighestDwordOffset; // Bits 63:32 of address
    DWORD           Reserved;
} IDT_ENTRY, *PIDT_ENTRY;
STATIC_ASSERT( sizeof( IDT_ENTRY ) == PREDEFINED_IDT_ENTRY_SIZE );
#pragma warning(pop)

typedef struct _IDT
{
    // IDTR(Limit) <-- SRC[0:15];
    WORD            Limit;

    // IDTR(Base)  <-- SRC[16:79];
    IDT_ENTRY*      Base;
} IDT, *PIDT;
STATIC_ASSERT( sizeof( IDT ) == PREDEFINED_IDT_SIZE );
#pragma pack(pop)

#define CREATE_DUMMY_DESC(desc)                      CreateInterruptDescriptor(NULL,TASK_GATE_ENTRY64,(desc),FALSE)

#define CREATE_TASK_GATE_DESC(addr,desc)             CreateInterruptDescriptor((addr),TASK_GATE_ENTRY64,(desc),TRUE)

// The ONLY difference between an interrupt and a trap gate is that the
// INTERRUPT gate clears the IF flag => no more interrupts in the current handler
#define CREATE_INTERRUPT_GATE_DESC(addr,desc)        CreateInterruptDescriptor((addr),INTERRUPT_GATE_ENTRY64,(desc),TRUE)
#define CREATE_TRAP_GATE_DESC(addr,desc)             CreateInterruptDescriptor((addr),TRAP_GATE_ENTRY64,(desc),TRUE)

//******************************************************************************
// Function:      CreateInterruptTable
// Description: Allocates and initializes the IDT.
// Returns:       STATUS
// Parameter:     IN WORD NumberOfEntries - Number of IDT entries
// Parameter:     OUT IDT* Idt            - Pointer to the newly allocated IDT
//******************************************************************************
STATUS
CreateInterruptTable(
    IN      WORD            NumberOfEntries,
    OUT     IDT*            Idt
);

//******************************************************************************
// Function:      CreateInterruptDescriptor
// Description: Initializes an interrupt descriptor.
// Returns:       STATUS
// Parameter:     IN PVOID EntryAddress
// Parameter:     IN BYTE GateType
// Parameter:     OUT IDT_ENTRY* Descriptor
// Parameter:     IN BOOLEAN Present
//******************************************************************************
STATUS
CreateInterruptDescriptor(
    IN_OPT  PVOID           EntryAddress,
    IN      BYTE            GateType,
    OUT     IDT_ENTRY*      Descriptor,
    IN      BOOLEAN         Present
);

#endif // _IDT_H_