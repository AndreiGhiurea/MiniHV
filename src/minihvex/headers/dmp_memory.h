#ifndef _DMP_MEMORY_H_
#define _DMP_MEMORY_H_

#include "minihv.h"
#include "vmmemory.h"

#define DUMP_LINE_WIDTH         0x10

#define LONG_ADDRESS_DIGITS     16

void
DumpMemory(
    IN      PVOID           LogicalAddress,
    IN      QWORD           Offset,
    IN      DWORD           Size,
    IN      BOOLEAN         DisplayAddress,
    IN      BOOLEAN         DisplayAscii
    );

void
DumpInt15MemoryMap( 
     IN INT15_MEMORY_MAP* MemoryMap
     );

#endif // _DMP_MEMORY_H_