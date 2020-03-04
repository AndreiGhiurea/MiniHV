#include "pit.h"

STATUS
PitSleep(
    IN    DWORD    MicrosecondsToSleep        
    )
{
    STATUS status;
    QWORD frequency;
    BYTE value;

    if( 0 == MicrosecondsToSleep )
    {
        return STATUS_INVALID_PARAMETER1;
    }

    if( MicrosecondsToSleep > 1 * SECONDS_IN_US )
    {
        return STATUS_TIMER_INVALID_FREQUENCY;
    }

    status = STATUS_SUCCESS;

    frequency = ( PIT_FREQUENCY_HZ * MicrosecondsToSleep ) / SECONDS_IN_US;

    if( frequency > MAX_WORD )
    {
        return STATUS_TIMER_INVALID_FREQUENCY;
    }

    // we read the current value from 0x61 (Status Control)
    value = __inbyte( STATUS_CONTROL_REG_PORT );

    // we disable the speaker(bit1)
    // 0b11111101
    value = ( value & (~(STATUS_SPEAKER_MASK) ) ) | STATUS_GATE2_MASK;

    // we update the control registers
    __outbyte( STATUS_CONTROL_REG_PORT, value );

    // we switch to mode 0
    __outbyte( PIT_COMMAND_REG_PORT, PIT_COMM_CHANNEL_2 | PIT_COMM_ACCESS_LO_HI | PIT_COMM_OPERATE_MODE_0 );

    __outbyte( PIT_CHANNEL2_DATA_PORT, WORD_LOW( frequency ) );
    __inbyte( 0x60 ); // short delay
    __outbyte( PIT_CHANNEL2_DATA_PORT, WORD_HIGH( frequency ) );


    //reset PIT one-shot counter (start counting)
    value = __inbyte( STATUS_CONTROL_REG_PORT ) & (~(STATUS_GATE2_MASK | STATUS_SPEAKER_MASK));
    __outbyte( STATUS_CONTROL_REG_PORT, value );

    // start countdown by setting bit 0 (GATE2)
    __outbyte( STATUS_CONTROL_REG_PORT, value | STATUS_GATE2_MASK );

    // wait for timer to finish
    // we need to wait for OUT2 to go active
    while( !( __inbyte(STATUS_CONTROL_REG_PORT ) & STATUS_OUT2_MASK ) );

    return status;
}