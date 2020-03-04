#ifndef _DMP_MULTIBOOT_H_
#define _DMP_MULTIBOOT_H_

#include "minihv.h"
#include "multiboot.h"

void 
DumpMemoryMap( 
    IN   MULTIBOOT_MEMORY_MAP_ENTRY* MemoryMap, 
    IN   DWORD NumberOfEntries 
);

void 
DumpMultiBootInformation(
    IN MULTIBOOT_INFORMATION* MultibootInformation
);

void
DumpParameters(
    IN ASM_PARAMETERS* Parameters 
);


#endif // _DMP_MULTIBOOT_H_