#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include "minihv.h"
#include "native/string.h"

#define BASE_VIDEO_ADDRESS          0xB8000UL
#define VIDEO_MEMORY_SIZE           (CHARS_PER_LINE*LINES_PER_SCREEN*BYTES_PER_CHAR)

// colors for printf
#define BLACK_COLOR                 0x7
#define RED_COLOR                   0x4

// screen size
#define CHARS_PER_LINE              80U
#define BYTES_PER_LINE              160U
#define LINES_PER_SCREEN            25U

#define BYTES_PER_CHAR              2

void
DispPreinitScreen(
    IN      BYTE    IndexOFFirstUsableRow,
    IN      BYTE    IndexOfLastUsableRow
    );

//******************************************************************************
// Function:      DispPrintString
// Description: Prints a buffer to the screen
// Returns:       void
// Parameter:     IN char* Buffer - NULL terminated buffer
// Parameter:     IN BYTE Color   - color to use
// NOTE:        Not MP safe.
//******************************************************************************
void
DispPrintString(
    IN_Z    char*    Buffer,
    IN      BYTE     Color
);

// Prints an error buffer to the display
#define perror(buf)         DispPrintString( buf, RED_COLOR );

//  Prints the C-style formatted string to the display
#define printf(buf,...)     AcquireLock( &(gGlobalData.LogData.DisplayLock) );                 \
                            sprintf( gGlobalData.LogData.DisplayBuffer, buf, __VA_ARGS__ );    \
                            DispPrintString( gGlobalData.LogData.DisplayBuffer, BLACK_COLOR ); \
                            ReleaseLock( &(gGlobalData.LogData.DisplayLock) );

#endif // _DISPLAY_H_