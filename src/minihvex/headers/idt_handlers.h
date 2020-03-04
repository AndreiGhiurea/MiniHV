#ifndef _IDT_HANDLERS_H_
#define _IDT_HANDLERS_H_

#include "minihv.h"
#include "idt.h"

typedef enum _EXCEPTION
{
    ExceptionDivideError                = 0,
    ExceptionDebugException,
    ExceptionNMI,
    ExceptionBreakpoint,
    ExceptionOverflow,
    ExceptionBoundRange,
    ExceptionInvalidOpcode,
    ExceptionDeviceNotAvailable,
    ExceptionDoubleFault,
    ExceptionCoprocOverrun,
    ExceptionInvalidTSS,
    ExceptionSegmentNotPresent,
    ExceptionStackFault,
    ExceptionGeneralProtection,
    ExceptionPageFault                  = 14,
    ExceptionX87FpuException            = 16,
    ExceptionAlignmentCheck,
    ExceptionMachineCheck,
    ExceptionSIMDFpuException,
    ExceptionVirtualizationException,
    ExceptionApicSpuriousInterrupt      = 39
} EXCEPTION;


#define     NO_OF_IDT_ENTRIES_TO_CREATE    (ExceptionApicSpuriousInterrupt)

#define     IVT_ENTRY_SIZE                  4
#define     IVT_LIMIT                       0x3FF

typedef struct _INTERRUPT_STACK_FORMAT
{
    QWORD                       Rip;
    QWORD                       CS;
    QWORD                       RFLAGS;
    QWORD                       Rsp;
    QWORD                       SS;
} INTERRUPT_STACK_FORMAT, *PINTERRUPT_STACK_FORMAT;

typedef struct _INTERRUPT_STACK_FORMAT_ERROR
{
    QWORD                       ErrorCode;
    INTERRUPT_STACK_FORMAT      StackValues;
} INTERRUPT_STACK_FORMAT_ERROR, *PINTERRUPT_STACK_FORMAT_ERROR;

//******************************************************************************
// Function:    InitializeIdtHandlers
// Description: Creates the IDT entries and loads the IDTR.
// Returns:     STATUS
// Parameter:   OUT IDT** Idt
//******************************************************************************
STATUS
InitializeIdtHandlers(
    OUT_PTR     PIDT*       Idt                 
);

#endif // _IDT_HANDLERS_H_