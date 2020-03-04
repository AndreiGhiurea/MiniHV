#include "pci.h"
#include "native/memory.h"
#include "hv_heap.h"
#include "data.h"
#include "dmp_pci.h"

LIST_ENTRY gPciDeviceList;

#define PCI_SET_CONFIG_REGISTER(X,Bus,Dev,Func,Reg)     \
            memzero(&(X), sizeof(PCI_CONFIG_REGISTER)); \
            (X).EnableBit = 1;                          \
            (X).BusNumber = (Bus);                      \
            (X).DeviceNumber = (Dev);                   \
            (X).FunctionNumber = (Func);                \
            (X).RegisterNumber = (Reg);


STATUS
PciRetrieveDevices(
void           
)
{
    STATUS status;
    DWORD currentBus;
    DWORD currentDevice;
    DWORD currentFunction;
    DWORD currentRegister;
    PCI_CONFIG_REGISTER addrMsg;
    DWORD dataRead;
    PCI_DEVICE_LIST_ENTRY* pDeviceEntry;

    InitializeListHead( &gPciDeviceList );

    status = STATUS_SUCCESS;
    pDeviceEntry = NULL;

    for( currentBus = 0; currentBus < MAX_BYTE; ++currentBus )
    {
        for( currentDevice = 0; currentDevice < ( 1 << 5 ); ++currentDevice )
        {
            for( currentFunction = 0; currentFunction < ( 1 << 3 ); ++currentFunction )
            {
                // 1 << 6
                for(currentRegister = 0; 
                    currentRegister < sizeof(PCI_DEVICE_HEADER); 
                    currentRegister = currentRegister + sizeof( PCI_CONFIG_REGISTER ) 
                        )
                {
                    PCI_SET_CONFIG_REGISTER( addrMsg, currentBus, currentDevice, currentFunction, currentRegister / sizeof(DWORD));
                    __outdword( PCI_CONFIG_ADDRESS, *( (DWORD*)&addrMsg ) );

                    dataRead =__indword( PCI_CONFIG_DATA);
                    
                    if( 0 == currentRegister )
                    {
                        if( MAX_DWORD == dataRead )
                        {
                            goto next_function;
                        }
                        
                        pDeviceEntry = HvAllocPoolWithTag( PoolAllocateZeroMemory, sizeof( PCI_DEVICE_LIST_ENTRY ), HEAP_PCI_TAG, 0 );
                        if( NULL == pDeviceEntry )
                        {
                            return STATUS_HEAP_INSUFFICIENT_RESOURCES;
                        }

                        InsertTailList( &gPciDeviceList, &( pDeviceEntry->ListEntry ) );
                    }

                    memcpy( (BYTE*)pDeviceEntry+currentRegister, &dataRead, sizeof(DWORD) );
                }

                DumpPciHeader(&pDeviceEntry->Header);
                pDeviceEntry = NULL;

            next_function:
                NOTHING
            }
        }
    }

    return status;
}