#ifndef _PCI_H_
#define _PCI_H_

#include "minihv.h"

#define PREDEFINED_PCI_DEVICE_HEADER_SIZE       64

#define PCI_CONFIG_ADDRESS                      0xCF8
#define PCI_CONFIG_DATA                         0xCFC

#pragma pack(push,1)
typedef struct _PCI_CONFIG_REGISTER
{
    DWORD   Reserved0               : 2;

    /// The RegisterNumber is a DWORD array index
    /// and not a BYTE array index :)
    DWORD   RegisterNumber          : 6;
    DWORD   FunctionNumber          : 3;
    DWORD   DeviceNumber            : 5;
    DWORD   BusNumber               : 8;
    DWORD   Reserved1               : 7;
    DWORD   EnableBit               : 1;
} PCI_CONFIG_REGISTER, *PPCI_CONFIG_REGISTER;
STATIC_ASSERT( sizeof( PCI_CONFIG_REGISTER ) == sizeof( DWORD) );

typedef struct _PCI_DEVICE_HEADER
{
    WORD    VendorID;
    WORD    DeviceID;

    // 0x04
    WORD    Command;
    WORD    Status;

    // 0x08
    BYTE    RevisionID;
    BYTE    ProgIF;
    BYTE    Subclass;
    BYTE    ClassCode;

    // 0x0C
    BYTE    CacheLineSize;
    BYTE    LatencyTimer;
    BYTE    HeaderType;
    BYTE    Bist;

    // 0x10
    DWORD   Bar[6];

    // 0x28
    DWORD   CarbusCISPointer;

    // 0x2C
    WORD    SubSystemVendorID;
    WORD    SubSystemID;

    // 0x30
    DWORD   ExpansionROMBaseAddress;

    // 0x34
    BYTE    CapabilitiesPointer;
    BYTE    Reserved[7];

    // 0x3C
    BYTE    InterruptLine;
    BYTE    InterruptPIN;
    BYTE    MinGrant;
    BYTE    MaxLatency;
} PCI_DEVICE_HEADER, *PPCI_DEVICE_HEADER;
STATIC_ASSERT( sizeof( PCI_DEVICE_HEADER ) == PREDEFINED_PCI_DEVICE_HEADER_SIZE );
#pragma pack(pop)

typedef struct _PCI_DEVICE_LIST
{
    PCI_DEVICE_HEADER       Header;
    LIST_ENTRY              ListEntry;
} PCI_DEVICE_LIST_ENTRY, *PPCI_DEVICE_LIST;

STATUS
PciRetrieveDevices(
    void           
);

#endif // _PCI_H_