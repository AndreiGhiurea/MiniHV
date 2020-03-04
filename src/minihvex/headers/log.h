#ifndef _LOG_H_
#define _LOG_H_

#include "minihv.h"
#include "native/string.h"
#include "serial.h"
#include "lock.h"

// 512 bytes should be enough
#define BUFF_MAX        0x200
//#define NO_COMM

#ifndef NO_COMM

#define LOG(buf,...)                AcquireLock( &(gGlobalData.LogData.SerialLock) );                  \
                                    LOCKLESS_LOG( buf, __VA_ARGS__ );                                  \
                                    ReleaseLock( &(gGlobalData.LogData.SerialLock) );

#define LOGP(buf,...)               LOG("[CPU:%02d]"##buf, CpuGetApicId(), __VA_ARGS__ )

#define LOGL(buf,...)               LOG("[%s][%d]"##buf, strrchr(__FILE__,'\\') + 1, __LINE__, __VA_ARGS__)

#define LOGPL(buf,...)              LOGL( "[CPU:%02d]"##buf, CpuGetApicId(), __VA_ARGS__ )

#define LOG_ERROR(buf,...)          LOGPL("[ERROR]"##buf, __VA_ARGS__)

#define SERIAL_LOG(buf,...)         AcquireLock( &(gGlobalData.LogData.SerialLock) );                  \
                                    sprintf( gGlobalData.LogData.SerialBuffer, buf, __VA_ARGS__ );     \
                                    SerialWriteBuffer( gGlobalData.LogData.SerialBuffer );             \
                                    ReleaseLock( &(gGlobalData.LogData.SerialLock) );

#define SERIAL_LOGL(buf,...)        SERIAL_LOG("[%s][%d]"##buf, strrchr(__FILE__,'\\') + 1, __LINE__, __VA_ARGS__)

#define LOCKLESS_LOG(buf,...)       sprintf( gGlobalData.LogData.SerialBuffer, buf, __VA_ARGS__ );     \
                                    SerialWriteBuffer( gGlobalData.LogData.SerialBuffer );

#define LOCKLESS_LOGP(buf,...)      LOCKLESS_LOG("[%s][%d][CPU:%02d]"##buf, strrchr(__FILE__,'\\') + 1, __LINE__,CpuGetApicId(), __VA_ARGS__)

#define LOG_FUNC_ERROR(func,status) LOGPL("Function [%s] failed with status 0x%X\n", func, status)

#else
// No communication
#define LOG(...)
#define LOGP(buf,...)
#define LOGL(buf,...)
#define LOGPL(buf,...)
#define LOG_ERROR(buf,...)
#define LOCKLESS_LOG(buf,...)
#define LOCKLESS_LOGP(buf,...)

#endif

// this structure keeps the buffers used for serial communication
// and for communication through video display
typedef struct _LOGGING_STRUCTURE
{
    char    DisplayBuffer[BUFF_MAX];
    LOCK    DisplayLock;

    char    SerialBuffer[BUFF_MAX];
    LOCK    SerialLock;
} LOGGING_STRUCTURE, *PLOGGING_STRUCTURE;


#endif // _LOG_H_