#include "dmp_pci.h"
#include "data.h"

void
DumpPciHeader(
    IN PCI_DEVICE_HEADER* Header
)
{
    DWORD i;

    if( NULL == Header )
    {
        return;
    }

    LOG( "Vendor ID: 0x%X\n", Header->VendorID );
    LOG( "Device ID: 0x%X\n", Header->DeviceID );
    LOG( "Class Code: 0x%X\n", Header->ClassCode );
    LOG( "Subclass: 0x%X\n", Header->Subclass );

    for (i = 0; i < 6; ++i)
    {
        LOG("Bar[%d] = 0x%x\n", i, Header->Bar[i]);
    }


    LOG( "\n");
}