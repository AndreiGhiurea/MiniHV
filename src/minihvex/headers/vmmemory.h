#ifndef _VM_MEMORY_H_
#define _VM_MEMORY_H_

#include "minihv.h"
#include "int15_memory.h"

#define INT15_E820                  0xE820
#define INT15_SIGNATURE             'SMAP'
#define INT15_ENTRY_SIZE            0x14

STATUS
PatchMemoryMap(
    INOUT   INT15_MEMORY_MAP*       MemoryMap,
    IN      QWORD                   BytesToCut,
    IN      QWORD                   StartAddress
    );

STATUS
SimulateInt15h(
    IN      INT15_MEMORY_MAP*       MemoryMap         
    );

STATUS
PatchInt15(
    void
    );

#endif // _VM_MEMORY_H_