#include "dmp_memory.h"
#include "strutils.h"
#include "log.h"
#include "data.h"

void
DumpMemory(
    IN      PVOID           LogicalAddress,
    IN      QWORD           Offset,
    IN      DWORD           Size,
    IN      BOOLEAN         DisplayAddress,
    IN      BOOLEAN         DisplayAscii
    )
{
    DWORD index;
    DWORD charPosition;
    PBYTE pCurAddress;
    char currentLine[3 * DUMP_LINE_WIDTH + 1];
    char currentAddress[LONG_ADDRESS_DIGITS + 1];
    char tempStr[3];
    char asciiLine[DUMP_LINE_WIDTH + 1];
    DWORD curValue;
    PVOID pointerToCurAddress;
    PBYTE pCurrentOffset;

    if ((NULL == LogicalAddress) || (0 == Size))
    {
        return;
    }

    pCurAddress = (PBYTE)LogicalAddress;
    charPosition = 0;
    pCurrentOffset = (PBYTE)Offset;

    for (index = 0; index < Size; ++index)
    {
        curValue = pCurAddress[index];
        snprintf(tempStr, 3, "%02x", curValue);

        currentLine[3 * charPosition] = tempStr[0];
        currentLine[3 * charPosition + 1] = tempStr[1];
        currentLine[3 * charPosition + 2] = ' ';

        snprintf(tempStr, 3, "%c", curValue);
        asciiLine[charPosition] = isascii(tempStr[0]) ? tempStr[0] : ' ';

        charPosition++;

        if (0 == ((index + 1) % DUMP_LINE_WIDTH))
        {
            currentLine[3 * charPosition] = '\0';
            asciiLine[charPosition] = '\0';
            charPosition = 0;

            pointerToCurAddress = pCurrentOffset + (index / DUMP_LINE_WIDTH) * DUMP_LINE_WIDTH;
            snprintf(currentAddress, 17, "%012X", pointerToCurAddress);

            if (DisplayAddress)
            {
                LOG("%s| ", currentAddress);
            }
            LOG("%s", currentLine);
            if (DisplayAscii)
            {
                LOG("|%s", asciiLine);
            }
            LOG("\n");
        }
    }

    // log the last line
    if (0 != charPosition)
    {
        currentLine[3 * charPosition] = '\0';
        LOG("%s\n", currentLine);
    }
}

void
DumpInt15MemoryMap( 
     IN INT15_MEMORY_MAP* MemoryMap
     )
{
    LIST_ENTRY* pCurEntry;
    INT15_MEMORY_MAP_LIST_ENTRY* pEntry;

    if( NULL == MemoryMap )
    {
        return;
    }

    LOG("INT 15H memory map\n");
    LOG( "Number of entries: %d\n", MemoryMap->NumberOfEntries );

    for(    pCurEntry = MemoryMap->MemoryMap.Flink; 
            pCurEntry != &( MemoryMap->MemoryMap ); 
            pCurEntry = pCurEntry->Flink )
    {
        pEntry = CONTAINING_RECORD( pCurEntry, INT15_MEMORY_MAP_LIST_ENTRY, ListEntry );

        LOG( "BaseAddress: 0x%X\n", pEntry->MemoryMapEntry.BaseAddress );
        LOG( "Size: 0x%X\n", pEntry->MemoryMapEntry.Length );
        LOG( "Type: %d\n", pEntry->MemoryMapEntry.Type );
        LOG("Extended attributed: 0x%x\n", pEntry->MemoryMapEntry.ExtendedAttributes);
    }
}