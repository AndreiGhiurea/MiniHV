#include "dmp_apic.h"
#include "log.h"
#include "data.h"

void
DumpMadtTable(
    IN      ACPI_TABLE_HEADER*       TableHeader
)
{
    DWORD i;
    DWORD actualTableLength;
    char buffer[MAX_PATH];
    BYTE* pData;
    ACPI_TABLE_MADT* pMadtTable;
    ACPI_MADT_LOCAL_APIC* pProc;
    DWORD curProcessor;

    if( NULL == TableHeader )
    {
        LOGL( "[ERROR] : TableHeader is NULL\n" );
        return;
    }

    i = 0;
    curProcessor = 0;
    actualTableLength = TableHeader->Length - sizeof( ACPI_TABLE_MADT );
    pData = NULL;
    pMadtTable = ( ACPI_TABLE_MADT* ) TableHeader;

    strncpy( buffer, TableHeader->Signature, ACPI_NAME_SIZE );
    LOG( "Table Name: %s\n", buffer );
    LOG( "Table Size: 0x%x\n", TableHeader->Length );
    LOG( "Data Size: 0x%x\n", actualTableLength );
    LOG( "Revision: 0x%x\n", TableHeader->Revision );
    LOG( "Checksum: 0x%x\n", TableHeader->Checksum );
    strncpy( buffer, TableHeader->OemId, ACPI_OEM_ID_SIZE );
    LOG( "OEM Id: %s\n", buffer );
    strncpy( buffer, TableHeader->OemTableId, ACPI_OEM_TABLE_ID_SIZE );
    LOG( "OEM Table Id: %s\n", buffer );
    LOG( "OEM Revision: 0x%x\n", TableHeader->OemRevision );
    strncpy( buffer, TableHeader->AslCompilerId, ACPI_NAME_SIZE );
    LOG( "ASL Compiler Id: %s\n", buffer );
    LOG( "ASL Revision: 0x%x\n", TableHeader->AslCompilerRevision );
    LOG( "Local APIC address: 0x%x\n", pMadtTable->Address);
    LOG( "Flags: 0x%x\n", pMadtTable->Flags );
    pData = ( BYTE* ) TableHeader + sizeof( ACPI_TABLE_MADT );

    while( i < actualTableLength )
    {
        pProc = ( ACPI_MADT_LOCAL_APIC* ) &(pData[i]);
        if( ACPI_MADT_TYPE_LOCAL_APIC == pProc->Header.Type )
        {
            LOG( "\n" );
            LOG( "CPU #%d\n", curProcessor );
            LOG( "APIC ID: 0x%x\n", pProc->ProcessorId );
            LOG( "Local ID: 0x%x\n", pProc->Id );
            LOG( "Flags: 0x%x\n", pProc->LapicFlags );
            curProcessor = curProcessor + 1;
        }

        i = i + pProc->Header.Length;
    }
}