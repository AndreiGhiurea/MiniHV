#ifndef _SERIAL_H_
#define _SERIAL_H_

#include "minihv.h"


// it is not physically posible to have more than 4
#define     MAX_NO_OF_SERIAL_PORTS          4

#define     BIOS_AREA_SERIAL_PORTS_ADDRESS  0x400

// Serial Port Register Offsets
#define     DATA_REG_OFFSET                 0x0     // Stores Data for both I&O
#define     INT_REG_OFFSET                  0x1     // Interrupt Register
#define     FIFO_REG_OFFSET                 0x2     // FIFO Register
#define     LINE_CREG_OFFSET                0x3     // Line Control Register
#define     MODEM_CREG_OFFSET               0x4     // Modem Control Register
#define     LSR_REG_OFFSET                  0x5     // Line Status Register
#define     MODEM_SREG_OFFSET               0x6     // Modem Status Register
#define     SCRATCH_REG_OFFSET              0x7     // Scratch Register
#define     RESERVED_REG_OFFSET             0x8

// these are valid only if DLAB is set
#define     LSB_DIV_OFFSET_VALUE            0x0     // LSB of BaudRate divisor
#define     MSB_DIV_OFFSET_VALUE            0x1     // MSB of BaudRate divisor

// LSR masks
#define     LSR_THR_READY                   (1<<5)  // data is ready to be sent
#define     LSR_OVERRUN_ERROR               (1<<1)

#define     DLAB_MASK                       (1<<7)

// FIFO masks
#define     FIFO_ENABLE                     (1<<0)
#define     FIFO_RECEIVER_RESET             (1<<1)
#define     FIFO_TRANSMIT_RESET             (1<<2)
#define     FIFO_RECEIVE_TRIG_0             (1<<6)
#define     FIFO_RECEIVE_TRIG_1             (1<<7)

// we will send 7 bits of data
#define     DATA_BITS                       2

// 1 stop bit
#define     STOP_BIT                        (0<<2)

// no parity bit
#define     PARITY_BIT                      (0<<3)

typedef struct _SERIAL_DEVICE
{
    // offset 0, DLAB == 0
    BYTE            DataRegister;

    // offset 1, DLAB == 0
    BYTE            InterruptEnableRegister;

    // offset 2 - 7, DLAB == x
    BYTE            FifoRegister;
    BYTE            LineControlRegister;
    BYTE            ModemControlRegister;
    BYTE            LineStatusRegister;
    BYTE            ModemStatusRegister;
    BYTE            ScratchRegister;

    // offset 0, DLAB == 1
    BYTE            BaudRateLSB;

    // offset 1, DLAB == 1
    BYTE            BaudRateMSB;
} SERIAL_DEVICE, PSERIAL_DEVICE;

//******************************************************************************
// Function:    SerialInitialize
// Description: Searches in the BIOS data area for any serial devices connected
//              and initializes communication on the first one found.
// Returns:     STATUS - Returns a warning if the serial connection cannot
//                       be initialized or a warning if serial was already 
//                       initialized
// Parameter:   void
//******************************************************************************
STATUS 
SerialInitialize( 
    void
);

//******************************************************************************
// Function:    SerialReinitialize
// Description: Reinitializes the current serial connection.
// Returns:       STATUS - failure if the serial wasn't initialized.
// Parameter:     void
//******************************************************************************
STATUS
SerialReinitialize(
void                   
);


//******************************************************************************
// Function:    SerialWriteBuffer
// Description:
// Returns:       void
// Parameter:     IN char * Buffer
//******************************************************************************
void 
SerialWriteBuffer( 
    IN_Z char* Buffer 
);


//******************************************************************************
// Function:    SerialWriteNBuffer
// Description:
// Returns:       void
// Parameter:     IN BYTE * Buffer
// Parameter:     IN DWORD BufferLength
//******************************************************************************
void 
SerialWriteNBuffer( 
    IN_READS_BYTES(BufferLength)    BYTE* Buffer, 
    IN                              DWORD BufferLength 
);

#endif // _SERIAL_H_