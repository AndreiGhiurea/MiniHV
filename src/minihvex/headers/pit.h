#ifndef _PIT_H_
#define _PIT_H_

#include "minihv.h"

#define PIT_CHANNEL0_DATA_PORT        0x40
#define PIT_CHANNEL1_DATA_PORT        0x41
#define PIT_CHANNEL2_DATA_PORT        0x42
#define PIT_COMMAND_REG_PORT        0x43


// I don't think this belongs only to the PIT
// theoretically this belongs to the PS/2 controller
#define STATUS_CONTROL_REG_PORT        0x61

#define STATUS_GATE2_MASK            (1<<0)
#define STATUS_SPEAKER_MASK            (1<<1)
#define STATUS_OUT1_MASK            (1<<4)
#define STATUS_OUT2_MASK            (1<<5)

// The Command Register flags
#define PIT_COMM_CHANNEL_0            (0x00<<6)
#define PIT_COMM_CHANNEL_1            (0x01<<6)
#define PIT_COMM_CHANNEL_2            (0x02<<6)

#define PIT_COMM_ACCESS_LATH        (0x00<<4)
#define PIT_COMM_ACCESS_LO_ONLY        (0x01<<4)
#define PIT_COMM_ACCESS_HI_ONLY        (0x02<<4)
#define PIT_COMM_ACCESS_LO_HI        (0x03<<4)

#define PIT_COMM_OPERATE_MODE_0        (0x00<<1)
#define PIT_COMM_OPERATE_MODE_1        (0x01<<1)
#define PIT_COMM_OPERATE_MODE_2        (0x02<<1)

#define PIT_COMM_BCD_ENABLED        1

// this is how many times a second the current count
// gets decremented
#define PIT_FREQUENCY_HZ            (1193182ULL)

#define SECONDS_IN_MS               (1000)
#define SECONDS_IN_US               (1000*1000)

//******************************************************************************
// Function:      PitSleep
// Description: Busy-Wait sleep.
// Returns:       STATUS
// Parameter:     IN DWORD MicrosecondsToSleep - Time to sleep
//******************************************************************************
STATUS
PitSleep(
    IN    DWORD    MicrosecondsToSleep        
);

#endif // _PIT_H_