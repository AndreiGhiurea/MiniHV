#include "display.h"
#include "native/memory.h"
#include "lock.h"
#include "log.h"
#include "data.h"

typedef struct _DISPLAY_DATA
{
    PBYTE       MappedScreenAddress;
    BYTE        CurrentColumn;
    BYTE        CurrentLine;
    BYTE        NumberOfLines;
    DWORD       TotalMemorySize;
} DISPLAY_DATA, *PDISPLAY_DATA;

static DISPLAY_DATA m_displayData;

static
void
_DispScrollScreen(
    void
    );

static
void
_DispClearScreen(
    void
    );

void
DispPreinitScreen(
    IN      BYTE    IndexOFFirstUsableRow,
    IN      BYTE    IndexOfFirstUnusableRow
    )
{
    memzero(&m_displayData, sizeof(DISPLAY_DATA));

    m_displayData.MappedScreenAddress = (PBYTE)PA2VA(BASE_VIDEO_ADDRESS + ( IndexOFFirstUsableRow * BYTES_PER_LINE ) );
    m_displayData.NumberOfLines = IndexOfFirstUnusableRow - IndexOFFirstUsableRow;
    m_displayData.TotalMemorySize = VIDEO_MEMORY_SIZE - ((LINES_PER_SCREEN - m_displayData.NumberOfLines) * BYTES_PER_LINE);

    _DispClearScreen();
}

void
DispPrintString
(
    IN_Z    char*    Buffer,
    IN      BYTE     Color
)
{
    DWORD index;
    BOOLEAN newline;
    WORD* screenAddress;

    // check parameters
    if( NULL == Buffer )
    {
        return;
    }

    // preinit variables
    screenAddress = NULL;
    index = 0;

    while( '\0' != Buffer[index] )
    {
        newline = FALSE;

        // check if we need to scroll the screen before writing
        if( m_displayData.NumberOfLines == m_displayData.CurrentLine )
        {
            _DispScrollScreen();
        }

        if( '\n' == Buffer[index] )
        {
            // newline
            newline = TRUE;
            goto end_it;
        }

        // warning C4312: 'type cast': conversion from 'unsigned long' to 'WORD *' of greater size
#pragma warning(suppress:4312)
        screenAddress = ( WORD* ) ( m_displayData.MappedScreenAddress + ( m_displayData.CurrentLine * BYTES_PER_LINE + m_displayData.CurrentColumn * BYTES_PER_CHAR ) );
        *screenAddress = BYTES_TO_WORD(Color,Buffer[index]);

        ++m_displayData.CurrentColumn;

        if( CHARS_PER_LINE == m_displayData.CurrentColumn )
        {
            // newline
            newline = TRUE;
            goto end_it;
        }

end_it:
        if( newline )
        {
            m_displayData.CurrentLine++;
            m_displayData.CurrentColumn = 0;
        }

        ++index;
    }

}

static
void
_DispScrollScreen(
    void
    )
{
    // we scroll the screen

    // warning C4312: 'type cast': conversion from 'DWORD' to 'PVOID' of greater size
#pragma warning(suppress:4312)
    memcpy((PVOID)m_displayData.MappedScreenAddress, (PVOID)(m_displayData.MappedScreenAddress + BYTES_PER_LINE), (m_displayData.NumberOfLines - 1) * BYTES_PER_LINE);

    // warning C4312: 'type cast': conversion from 'DWORD' to 'PVOID' of greater size
#pragma warning(suppress:4312)
    memzero((PVOID)(m_displayData.MappedScreenAddress + (m_displayData.NumberOfLines - 1) * BYTES_PER_LINE), BYTES_PER_LINE);

    m_displayData.CurrentLine--;
}

static
void
_DispClearScreen(
    void
    )
{
    // clear the screen
    memzero(m_displayData.MappedScreenAddress, m_displayData.TotalMemorySize);

    // update current line to start toping at top of the screen
    m_displayData.CurrentLine = 0;
}