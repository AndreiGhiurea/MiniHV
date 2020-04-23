#include "intro.h"
#include "vmguest.h"
#include "data.h"
#include "vmx.h"
#include "mzpe.h"

STATUS
IntFindKernelBase(
    _In_ QWORD KernelAddr
)
{
    STATUS status = STATUS_SUCCESS;
    BOOLEAN found = FALSE;
    DWORD nrOfPages = 0;
    QWORD kernelAddr = KernelAddr;
    PBYTE hostVa = NULL;
    PVOID hostPa = NULL;

    if (0 == KernelAddr)
    {
        return STATUS_UNSUCCESSFUL;
    }

    kernelAddr &= PAGE_MASK;
    while (!found && nrOfPages < 100)
    {
        status = GuestVAToHostVA(kernelAddr, &hostPa, &hostVa);
        if (!SUCCEEDED(status))
        {
            LOGL("GuestVAToHostVA failed with status: 0x%x\n", status);
            return STATUS_UNSUCCESSFUL;
        }

        if (hostVa[0] == 'M' && hostVa[1] == 'Z')
        {
            LOGL("Kernel Base found at: 0x%x\n", kernelAddr);
            gGlobalData.Intro.KernelBase = kernelAddr;
            found = TRUE;
        }

        status = UnmapMemory(hostPa, PAGE_SIZE);
        if (!SUCCEEDED(status))
        {
            LOGL("UnmapMemory failed with status: 0x%x\n", status);
        }

        if (found)
        {
            return STATUS_SUCCESS;
        }

        kernelAddr -= PAGE_SIZE;
    }

    return STATUS_UNSUCCESSFUL;
}

STATUS
IntGetActiveProcessesList(
    _In_ DWORD BufferSize,
    _Inout_ CHAR* Buffer)
{
    STATUS status = STATUS_SUCCESS;
    QWORD initialSystemProcessSymbol = 0;
    DWORD initialSystemEprocess = 0;
    PVOID hostVa = NULL;
    PVOID hostPa = NULL;
    DWORD processPid = 0;
    BYTE processName[16] = { 0 };
    DWORD eprocessAddr = 0;
    DWORD copiedSize = 0;
    DWORD listEntryAddr = 0;
    LIST_ENTRY listEntry = { 0 };
    CHAR tempBuffer[MAX_PATH];

    if (gGlobalData.Intro.KernelBase == 0)
    {
        LOGL("Intro not initialized\n");
        return STATUS_UNSUCCESSFUL;
    }

    status = MzpeFindExport(gGlobalData.Intro.KernelBase, "PsInitialSystemProcess", &initialSystemProcessSymbol);
    if (!SUCCEEDED(status))
    {
        LOGL("MzpeFindExport failed with status: 0x%x\n", status);
        return STATUS_UNSUCCESSFUL;
    }

    status = GuestReadDword(initialSystemProcessSymbol, &initialSystemEprocess);
    if (!SUCCEEDED(status))
    {
        LOGL("GuestReadDword failed with status: 0x%x\n", status);
        return STATUS_UNSUCCESSFUL;
    }

    LOGL("Initial System Eprocess: 0x%x\n", initialSystemEprocess);

    eprocessAddr = initialSystemEprocess;

    _get_next_process:

    status = GuestVAToHostVA(eprocessAddr, &hostPa, &hostVa);
    if (!SUCCEEDED(status))
    {
        LOGL("GuestVAToHostVA failed with status: 0x%x\n", status);
        return STATUS_UNSUCCESSFUL;
    }
    
    processPid = *(PDWORD)((BYTE*)hostVa + 0xb4);
    memcpy(processName, (BYTE*)hostVa + 0x17c, 15);

    listEntryAddr = (*(PDWORD)((BYTE*)hostVa + 0xb8));
    status = GuestReadSize(listEntryAddr, sizeof(LIST_ENTRY), &listEntry);
    if (!SUCCEEDED(status))
    {
        LOGL("GuestReadSize failed: 0x%x\n", status);
    }

    sprintf(tempBuffer, "%s PID: %d\n", processName, processPid);
    sprintf(Buffer + copiedSize, tempBuffer);
    copiedSize += strlen(tempBuffer);

    eprocessAddr = ((QWORD)listEntry.Flink - 0xb8) & DWORD_MASK;

    status = UnmapMemory(hostPa, PAGE_SIZE);
    if (!SUCCEEDED(status))
    {
        LOGL("UnmapMemory failed with status: 0x%x\n", status);
    }

    if ((copiedSize < BufferSize) &&
        (eprocessAddr != initialSystemEprocess))
    {
        goto _get_next_process;
    }

    return STATUS_SUCCESS;
}