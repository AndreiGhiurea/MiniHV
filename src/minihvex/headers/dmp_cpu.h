#ifndef _DMP_CPU_H_
#define _DMP_CPU_H_

#include "minihv.h"
#include "cpumu.h"
#include "list.h"

STATUS
DumpPhysicalCpu(
    IN  LIST_ENTRY*         CpuEntry,
    IN  PVOID               Context
    );

void
DumpRegisterArea(
    IN  REGISTER_AREA*              ProcessorState
    );

STATUS
DumpProcessorState(
    IN  COMPLETE_PROCESSOR_STATE*    ProcessorState
    );

#endif // _DMP_CPU_H_