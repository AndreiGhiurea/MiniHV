// ACPICA OSL implementation

#include "minihv.h"
#include "acpi.h"
#include "log.h"
#include "hv_heap.h"
#include "paging_tables.h"
#include "data.h"


#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsInitialize
ACPI_STATUS
AcpiOsInitialize (
    void)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsTerminate
ACPI_STATUS
AcpiOsTerminate (
    void)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetRootPointer
ACPI_PHYSICAL_ADDRESS
AcpiOsGetRootPointer (
    void)
{
    ACPI_SIZE Ret;

    LOGL( "Entering function %s\n", __FUNCTION__ );


    AcpiFindRootPointer(&Ret);
    return Ret;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsPredefinedOverride
ACPI_STATUS
AcpiOsPredefinedOverride (
    const ACPI_PREDEFINED_NAMES *InitVal,
    ACPI_STRING                 *NewVal)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsTableOverride
ACPI_STATUS
AcpiOsTableOverride (
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_TABLE_HEADER       **NewTable)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    *NewTable = NULL;

    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsPhysicalTableOverride
ACPI_STATUS
AcpiOsPhysicalTableOverride (
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_PHYSICAL_ADDRESS   *NewAddress,
    UINT32                  *NewTableLength)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    *NewAddress = 0;
    *NewTableLength = 0;

    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCreateLock
ACPI_STATUS
AcpiOsCreateLock (
    ACPI_SPINLOCK           *OutHandle)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsDeleteLock
void
AcpiOsDeleteLock (
    ACPI_SPINLOCK           Handle)
{
    return;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsAcquireLock
ACPI_CPU_FLAGS
AcpiOsAcquireLock (
    ACPI_SPINLOCK           Handle)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return 0;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReleaseLock
void
AcpiOsReleaseLock (
    ACPI_SPINLOCK           Handle,
    ACPI_CPU_FLAGS          Flags)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCreateSemaphore
ACPI_STATUS
AcpiOsCreateSemaphore (
    UINT32                  MaxUnits,
    UINT32                  InitialUnits,
    ACPI_SEMAPHORE          *OutHandle)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsDeleteSemaphore
ACPI_STATUS
AcpiOsDeleteSemaphore (
    ACPI_SEMAPHORE          Handle)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWaitSemaphore
ACPI_STATUS
AcpiOsWaitSemaphore (
    ACPI_SEMAPHORE          Handle,
    UINT32                  Units,
    UINT16                  Timeout)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsSignalSemaphore
ACPI_STATUS
AcpiOsSignalSemaphore (
    ACPI_SEMAPHORE          Handle,
    UINT32                  Units)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif

#if (ACPI_MUTEX_TYPE != ACPI_BINARY_SEMAPHORE)

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCreateMutex
ACPI_STATUS
AcpiOsCreateMutex (
    ACPI_MUTEX              *OutHandle)
{
    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsDeleteMutex
void
AcpiOsDeleteMutex (
    ACPI_MUTEX              Handle)
{
    return;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsAcquireMutex
ACPI_STATUS
AcpiOsAcquireMutex (
    ACPI_MUTEX              Handle,
    UINT16                  Timeout)
{
    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReleaseMutex
void
AcpiOsReleaseMutex (
    ACPI_MUTEX              Handle)
{
    return;
}
#endif

#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsAllocate
void *
AcpiOsAllocate (
    ACPI_SIZE               Size)
{
    LOG( "AcpiOsAllocate about to allocate 0x%X bytes\n", Size );

    return HvAllocPoolWithTag( PoolAllocatePanicIfFail, ( DWORD ) Size, HEAP_ACPI_TAG, 0 );
}
#endif


#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsFree
void
AcpiOsFree (
    void*                   Memory)
{
    LOG( "AcpiOsFree about to free memory at address 0x%X\n", Memory );

    HvFreePoolWithTag( Memory, HEAP_ACPI_TAG );
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsMapMemory
void *
AcpiOsMapMemory (
    ACPI_PHYSICAL_ADDRESS   Where,
    ACPI_SIZE               Length)
{
    LOG( "AcpiOsMapMemory about to map 0x%X bytes starting at PA 0x%X\n", Length, Where );

    return MapMemory( ( PVOID ) Where, ( DWORD ) Length );
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsUnmapMemory
void
AcpiOsUnmapMemory (
    void                    *LogicalAddress,
    ACPI_SIZE               Size)
{
    LOG( "AcpiOsUnmapMemory about to unmap 0x%X bytes starting at VA 0x%X\n", Size, LogicalAddress );

    //UnmapMemory( LogicalAddress, ( DWORD ) Size );
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetPhysicalAddress
ACPI_STATUS
AcpiOsGetPhysicalAddress (
    void                    *LogicalAddress,
    ACPI_PHYSICAL_ADDRESS   *PhysicalAddress)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif


/*
 * Memory/Object Cache
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCreateCache
ACPI_STATUS
AcpiOsCreateCache (
    char                    *CacheName,
    UINT16                  ObjectSize,
    UINT16                  MaxDepth,
    ACPI_CACHE_T            **ReturnCache)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsDeleteCache
ACPI_STATUS
AcpiOsDeleteCache (
    ACPI_CACHE_T            *Cache)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsPurgeCache
ACPI_STATUS
AcpiOsPurgeCache (
    ACPI_CACHE_T            *Cache)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsAcquireObject
void *
AcpiOsAcquireObject (
    ACPI_CACHE_T            *Cache)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return NULL;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReleaseObject
ACPI_STATUS
AcpiOsReleaseObject (
    ACPI_CACHE_T            *Cache,
    void                    *Object)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif


/*
 * Interrupt handlers
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsInstallInterruptHandler
ACPI_STATUS
AcpiOsInstallInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine,
    void                    *Context)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsRemoveInterruptHandler
ACPI_STATUS
AcpiOsRemoveInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif


/*
 * Threads and Scheduling
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetThreadId
ACPI_THREAD_ID
AcpiOsGetThreadId (
    void)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return 0;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsExecute
ACPI_STATUS
AcpiOsExecute (
    ACPI_EXECUTE_TYPE       Type,
    ACPI_OSD_EXEC_CALLBACK  Function,
    void                    *Context)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWaitEventsComplete
void
AcpiOsWaitEventsComplete (
    void)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsSleep
void
AcpiOsSleep (
    UINT64                  Milliseconds)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsStall
void
AcpiOsStall (
    UINT32                  Microseconds)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return;
}
#endif


/*
 * Platform and hardware-independent I/O interfaces
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReadPort
ACPI_STATUS
AcpiOsReadPort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  *Value,
    UINT32                  Width)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWritePort
ACPI_STATUS
AcpiOsWritePort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  Value,
    UINT32                  Width)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif


/*
 * Platform and hardware-independent physical memory interfaces
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReadMemory
ACPI_STATUS
AcpiOsReadMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT64                  *Value,
    UINT32                  Width)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWriteMemory
ACPI_STATUS
AcpiOsWriteMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT64                  Value,
    UINT32                  Width)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif


/*
 * Platform and hardware-independent PCI configuration space access
 * Note: Can't use "Register" as a parameter, changed to "Reg" --
 * certain compilers complain.
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReadPciConfiguration
ACPI_STATUS
AcpiOsReadPciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Reg,
    UINT64                  *Value,
    UINT32                  Width)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWritePciConfiguration
ACPI_STATUS
AcpiOsWritePciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Reg,
    UINT64                  Value,
    UINT32                  Width)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif


/*
 * Miscellaneous
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReadable
BOOLEAN
AcpiOsReadable (
    void                    *Pointer,
    ACPI_SIZE               Length)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );
    return TRUE;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWritable
BOOLEAN
AcpiOsWritable (
    void                    *Pointer,
    ACPI_SIZE               Length)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return TRUE;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetTimer
UINT64
AcpiOsGetTimer (
    void)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return 0;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsSignal
ACPI_STATUS
AcpiOsSignal (
    UINT32                  Function,
    void                    *Info)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif


/*
 * Debug print routines
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsPrintf
void ACPI_INTERNAL_VAR_XFACE
AcpiOsPrintf (
    const char              *Format,
    ...)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsVprintf
void
AcpiOsVprintf (
    const char              *Format,
    va_list                 Args)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsRedirectOutput
void
AcpiOsRedirectOutput (
    void                    *Destination)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return;
}
#endif


/*
 * Debug input
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetLine
ACPI_STATUS
AcpiOsGetLine (
    char                    *Buffer,
    UINT32                  BufferLength,
    UINT32                  *BytesRead)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif


/*
 * Obtain ACPI table(s)
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetTableByName
ACPI_STATUS
AcpiOsGetTableByName (
    char                    *Signature,
    UINT32                  Instance,
    ACPI_TABLE_HEADER       **Table,
    ACPI_PHYSICAL_ADDRESS   *Address)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetTableByIndex
ACPI_STATUS
AcpiOsGetTableByIndex (
    UINT32                  Index,
    ACPI_TABLE_HEADER       **Table,
    UINT32                  *Instance,
    ACPI_PHYSICAL_ADDRESS   *Address)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetTableByAddress
ACPI_STATUS
AcpiOsGetTableByAddress (
    ACPI_PHYSICAL_ADDRESS   Address,
    ACPI_TABLE_HEADER       **Table)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif


/*
 * Directory manipulation
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsOpenDirectory
void *
AcpiOsOpenDirectory (
    char                    *Pathname,
    char                    *WildcardSpec,
    char                    RequestedFileType)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return NULL;
}
#endif


#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetNextFilename
char *
AcpiOsGetNextFilename (
    void                    *DirHandle)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return NULL;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCloseDirectory
void
AcpiOsCloseDirectory (
    void                    *DirHandle)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return;
}
#endif


/*
 * File I/O and related support
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsOpenFile
ACPI_FILE
AcpiOsOpenFile (
    const char              *Path,
    UINT8                   Modes)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return NULL;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCloseFile
void
AcpiOsCloseFile (
    ACPI_FILE               File);
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReadFile
int
AcpiOsReadFile (
    ACPI_FILE               File,
    void                    *Buffer,
    ACPI_SIZE               Size,
    ACPI_SIZE               Count)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return 0;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWriteFile
int
AcpiOsWriteFile (
    ACPI_FILE               File,
    void                    *Buffer,
    ACPI_SIZE               Size,
    ACPI_SIZE               Count)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return 0;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetFileOffset
long
AcpiOsGetFileOffset (
    ACPI_FILE               File)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return 0;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsSetFileOffset
ACPI_STATUS
AcpiOsSetFileOffset (
    ACPI_FILE               File,
    long                    Offset,
    UINT8                   From)
{
    LOGL( "Entering function %s\n", __FUNCTION__ );

    return AE_OK;
}
#endif

#undef memset
#ifdef NDEBUG
#pragma function(memset)
#endif
_At_buffer_(address, i, size, _Post_satisfies_(((PBYTE)address)[i] == value))
void*
memset(
    OUT_WRITES_BYTES_ALL(size)  PVOID address,
    IN                          BYTE value,
    IN                          DWORD size
)
{
    cl_memset(address, value, size);

    return address;
}

#undef memcpy
#ifdef NDEBUG
#pragma function(memcpy)
#endif
_At_buffer_(Destination,i, Count,
            _Post_satisfies_(((PBYTE)Destination)[i] == ((PBYTE)Source)[i]))
void*
memcpy(
    OUT_WRITES_BYTES_ALL(Count) PVOID   Destination,
    IN_READS(Count)             PVOID   Source,
    IN                          QWORD   Count
    )
{
    cl_memcpy(Destination, Source, Count);

    return Destination;
}
