#ifndef _VMEXIT_STATS_H_
#define _VMEXIT_STATS_H_

#include "vmexit_reason.h"

#pragma pack(push,8)
typedef struct _IO_UNI_OPERATION_STATS
{
    QWORD                       ByteAccess;
    QWORD                       WordAccess;
    QWORD                       DwordAccess;
} IO_UNI_OPERATION_STATS, *PIO_UNI_OPERATION_STATS;

typedef struct _IO_OPERATION_STATS
{
    IO_UNI_OPERATION_STATS      InputOperations;
    IO_UNI_OPERATION_STATS      OutputOperations;
} IO_OPERATION_STATS, *PIO_OPERATION_STATS;

typedef struct _CONTROL_REGISTER_STATS
{
    QWORD                       MovesFrom;
    QWORD                       MovesTo;
} CONTROL_REGISTER_STATS, *PCONTROL_REGISTER_STATS;

typedef struct _CONTROL_ACCESS_STATS
{
   CONTROL_REGISTER_STATS       Cr0Stats;
   CONTROL_REGISTER_STATS       Cr3Stats;
   CONTROL_REGISTER_STATS       Cr4Stats;
} CONTROL_ACCESS_STATS, *PCONTROL_ACCESS_STATS;

typedef struct _VMEXIT_STATS
{
    volatile QWORD              ExitCount[VM_EXIT_RESERVED];
    CONTROL_ACCESS_STATS        ControlAccesStats;
    IO_OPERATION_STATS          IoOperations;
    QWORD                       TotalTimeInExitHandler;
} VMEXIT_STATS, *PVMEXIT_STATS;
#pragma pack(pop)

#endif // _VMEXIT_STATS_H_