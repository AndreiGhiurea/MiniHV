#include "intro.h"
#include "vmguest.h"
#include "data.h"
#include "vmx.h"

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
        status = GuestVAToHostVA((PVOID)kernelAddr, &hostPa, &hostVa);
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
IntGetActiveEprocess(DWORD* Pid
)
{
    STATUS status = STATUS_SUCCESS;
    QWORD fsBase = VmxRead(VMCS_GUEST_FS_BASE);
    PVOID hostPa = NULL;
    PBYTE hostVa = NULL;
    QWORD currentThread = 0;
    QWORD currentProcess = 0;
    DWORD processPid = 0;

    status = GuestVAToHostVA((PVOID)fsBase, &hostPa, &hostVa);
    if (!SUCCEEDED(status))
    {
        LOGL("GuestVAToHostVA failed with status: 0x%x\n", status);
        return STATUS_UNSUCCESSFUL;
    }

    LOGL("FS Base: %x\n", fsBase);
    LOGL("FS DWORD: %x\n", *(PDWORD)(hostVa));

    currentThread = *(PDWORD)(hostVa + 0x124);
    LOGL("Current Thread: %x\n", currentThread);

    status = UnmapMemory(hostPa, PAGE_SIZE);
    if (!SUCCEEDED(status))
    {
        LOGL("UnmapMemory failed with status: 0x%x\n", status);
        return STATUS_UNSUCCESSFUL;
    }

    status = GuestVAToHostVA((PVOID)currentThread, &hostPa, &hostVa);
    if (!SUCCEEDED(status))
    {
        LOGL("GuestVAToHostVA failed with status: 0x%x\n", status);
        return STATUS_UNSUCCESSFUL;
    }

    currentProcess = *(PDWORD)(hostVa + 0x150);

    status = UnmapMemory(hostPa, PAGE_SIZE);
    if (!SUCCEEDED(status))
    {
        LOGL("UnmapMemory failed with status: 0x%x\n", status);
        return STATUS_UNSUCCESSFUL;
    }

    status = GuestVAToHostVA((PVOID)currentProcess, &hostPa, &hostVa);
    if (!SUCCEEDED(status))
    {
        LOGL("GuestVAToHostVA failed with status: 0x%x\n", status);
        return STATUS_UNSUCCESSFUL;
    }

    processPid = *(PDWORD)(hostVa + 0xb4);

    LOGL("Process PID: %d\n", processPid);

    *Pid = processPid;

    return STATUS_SUCCESS;
}