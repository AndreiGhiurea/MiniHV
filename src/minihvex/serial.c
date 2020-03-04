#include "serial.h"
#include "data.h"




//******************************************************************************
// Function:    SerialOut
// Description: Outputs a byte of data through the serial port.
// Returns:       void
// Parameter:     IN BYTE Data - byte to output.
//******************************************************************************
__forceinline
static
void 
SerialOut(
    IN BYTE Data 
)
{
    if( 0 == gGlobalData.SerialPortNumber )
    {
        return;
    }

    // we wait for the data register to be ready to receive new data
    while( ( __inbyte( gGlobalData.SerialPortNumber + LSR_REG_OFFSET ) & LSR_THR_READY ) == 0 );

    // we output the byte
    __outbyte( gGlobalData.SerialPortNumber, Data );
}

STATUS 
SerialInitialize( 
    void
)
{
    STATUS status;
    WORD* mappedBiosAreaSerialPorts;
    WORD currentPortAddress;
    int i;

    status = STATUS_SUCCESS;
    mappedBiosAreaSerialPorts = NULL;
    currentPortAddress = 0;

    if( 0 != gGlobalData.SerialPortNumber )
    {
        // return an error warning
        return STATUS_COMM_SERIAL_ALREADY_INITIALIZED;
    }

    mappedBiosAreaSerialPorts = MapMemory((PVOID)BIOS_AREA_SERIAL_PORTS_ADDRESS, sizeof(WORD)*MAX_NO_OF_SERIAL_PORTS);
    if( NULL == mappedBiosAreaSerialPorts )
    {
        status = STATUS_MEMORY_CANNOT_BE_MAPPED;
        goto cleanup;
    }

    for( i = 0; i < MAX_NO_OF_SERIAL_PORTS; ++i )
    {
        currentPortAddress = mappedBiosAreaSerialPorts[i];
        if( 0 != currentPortAddress )
        {
            break;
        }
    }

    if( 0 == currentPortAddress )
    {
        status = STATUS_COMM_SERIAL_NO_PORTS_AVAILABLE;
        goto cleanup;
    }

    gGlobalData.SerialPortNumber = currentPortAddress;

    // Disable all interrupts
    __outbyte( currentPortAddress + INT_REG_OFFSET, 0x00);    
    __outbyte( currentPortAddress + LINE_CREG_OFFSET, DLAB_MASK );

    // set baud rate divisor to 1 => BaudRate = 115200
    __outbyte( currentPortAddress + LSB_DIV_OFFSET_VALUE, 1 );
    __outbyte( currentPortAddress + MSB_DIV_OFFSET_VALUE, 0 );

    // 7 data bits, no parity, one stop bit
    __outbyte( currentPortAddress + LINE_CREG_OFFSET, PARITY_BIT | STOP_BIT | DATA_BITS );

    // we enable FIFO
    // we would theoretically be interrupted every 14 characters received but because we have interrupts disabled
    // this will never happen
    __outbyte( currentPortAddress + FIFO_REG_OFFSET, FIFO_RECEIVE_TRIG_1 | FIFO_RECEIVE_TRIG_0 | FIFO_TRANSMIT_RESET | FIFO_RECEIVER_RESET | FIFO_ENABLE );

cleanup:
    if( NULL != mappedBiosAreaSerialPorts)
    {
        STATUS statusSup;

        statusSup = UnmapMemory(mappedBiosAreaSerialPorts, sizeof(WORD)*MAX_NO_OF_SERIAL_PORTS);
        ASSERT(SUCCEEDED(statusSup));
    }

    return status;
}

STATUS
SerialReinitialize(
void                   
)
{
    if( 0 == gGlobalData.SerialPortNumber )
    {
        return STATUS_COMM_SERIAL_NOT_INITIALIZED;
    }

    gGlobalData.SerialPortNumber = 0;

    return SerialInitialize();
}

void 
SerialWriteBuffer( 
    IN_Z char* Buffer 
)
{
    DWORD i;

    if (0 == gGlobalData.SerialPortNumber)
    {
        return;
    }

    if( NULL == Buffer )
    {
        return;
    }

    i = 0;

    while( '\0' != Buffer[i] )
    {
        SerialOut( Buffer[i] );
        ++i;
    }
}

void
SerialWriteNBuffer(
    IN_READS_BYTES(BufferLength)    BYTE* Buffer,
    IN                              DWORD BufferLength
    )
{
    DWORD i;

    if (0 == gGlobalData.SerialPortNumber)
    {
        return;
    }

    if( NULL == Buffer )
    {
        return;
    }

    for( i = 0; i < BufferLength; ++i )
    {
        SerialOut( Buffer[i] );
    }
}