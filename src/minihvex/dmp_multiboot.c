#include "dmp_multiboot.h"
#include "log.h"
#include "display.h"
#include "data.h"

void 
DumpMemoryMap( 
    IN   MULTIBOOT_MEMORY_MAP_ENTRY* MemoryMap, 
    IN   DWORD NumberOfEntries 
)
{
    DWORD i;

    if( ( NULL == MemoryMap ) || ( 0 == NumberOfEntries ) )
    {
        return;
    }

    for( i = 0; i < NumberOfEntries; ++i )
    {
        LOG( "Memory region %d:\n", i );
        LOG( "Base Address: 0x%X\n", MemoryMap[i].BaseAddress );
        LOG( "Memory Size: %D kb\n", MemoryMap[i].MemoryLength / KB_SIZE  );
        LOG( "Memory Type: %d\n", MemoryMap[i].Type );
        LOG( "\n" );
    }
}

void 
DumpMultiBootInformation(
    IN MULTIBOOT_INFORMATION* MultibootInformation
)
{
    DWORD noOfMemoryMapEntries;

    if( NULL == MultibootInformation )
    {
        return;
    }

    noOfMemoryMapEntries = 0;

    LOG( "\n" );
    LOG( "-----------------------------\n" );
    LOG( "Multiboot structure at: 0x%X\n", MultibootInformation);
    LOG( "Flags: 0b%b\n", MultibootInformation->Flags );
    LOG( "Lower Memory Size: %D bytes\n", MultibootInformation->LowerMemorySize * KB_SIZE );
    LOG( "Higher Memory SIze: %D bytes\n", MultibootInformation->HigherMemorySize * KB_SIZE );
    LOG("Boot device: 0x%x\n", MultibootInformation->BootDevice);
    // command line
    // module information
    LOG( "Memory map address: 0x%X\n", MultibootInformation->MemoryMapAddress );
    LOG( "Size of memory map: %d bytes\n", MultibootInformation->MemoryMapSize );
    noOfMemoryMapEntries = MultibootInformation->MemoryMapSize / sizeof( MULTIBOOT_MEMORY_MAP_ENTRY );

    // warning C4312: 'type cast': conversion from 'const DWORD' to 'MULTIBOOT_MEMORY_MAP_ENTRY *' of greater size
#pragma warning(suppress:4312)
    DumpMemoryMap( ( MULTIBOOT_MEMORY_MAP_ENTRY* ) MultibootInformation->MemoryMapAddress, noOfMemoryMapEntries );

    // warning C4312: 'type cast': conversion from 'const DWORD' to 'char *' of greater size
#pragma warning(suppress:4312)
    LOG( "Loader name: %s\n", ( char* ) MultibootInformation->BootLoaderName );
    LOG( "-----------------------------" );
    LOG( "\n" );
}

void
DumpParameters(
    IN ASM_PARAMETERS* Parameters 
)
{
    if( NULL == Parameters )
    {
        return;
    }

    DumpMultiBootInformation( Parameters->MultibootInformation );
}