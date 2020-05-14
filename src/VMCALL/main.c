#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>

VOID _cdecl _VmcallToHV(
    DWORD InputBufferSize,
    PVOID InputBuffer,
    DWORD OutputBufferSize,
    VOID* OutputBuffer,
    VOID* Context
);

BOOL CompareProcessList(CHAR ProcessNames[255][16], DWORD* ProcessPids, DWORD NrOfProcesses)
{
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    DWORD windowsProcessPids[255];
    
    // Take a snapshot of all processes in the system.
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        printf("CreateToolhelp32Snapshot (of processes)\n");
        return FALSE;
    }

    // Set the size of the structure before using it.
    pe32.dwSize = sizeof(PROCESSENTRY32);

    // Retrieve information about the first process,
    // and exit if unsuccessful
    if (!Process32First(hProcessSnap, &pe32))
    {
        printf("Process32First\n"); // show cause of failure
        CloseHandle(hProcessSnap);          // clean the snapshot object
        return FALSE;
    }

    // Now walk the snapshot of processes, and
    // display information about each process in turn
    DWORD nr = 0;
    do
    {
        // Skip this as we do in HV
        if (pe32.th32ProcessID == 0)
        {
            continue;
        }

        windowsProcessPids[nr] = pe32.th32ProcessID;
        nr++;
    } while (Process32Next(hProcessSnap, &pe32));

    printf("Got %d process from Windows\n", nr);


    BOOL found = FALSE;
    for (DWORD i = 0; i < NrOfProcesses; i++)
    {
        found = FALSE;
        for (DWORD j = 0; j < nr; j++)
        {
            if (ProcessPids[i] == windowsProcessPids[j])
            {
                found = TRUE;
            }
        }

        if (found == FALSE)
        {
            printf("Hidden Process: %s (%d)\n", ProcessNames[i], ProcessPids[i]);
        }
    }

    CloseHandle(hProcessSnap);
    return TRUE;
}


int main()
{
    CHAR* outputBuffer = NULL;
    DWORD context = 0;
    outputBuffer = (CHAR*)malloc(0x1000);
    if (NULL == outputBuffer)
    {
        printf("Malloc failed\n");
        return 1;
    }

    _VmcallToHV(0, NULL, 0x1000, outputBuffer, &context);

    CHAR processNames[255][16];
    DWORD processPids[255];
    DWORD nrOfProcesses = 0;

    printf("Should have %d processes from HV\n", context);
    
    const CHAR s[2] = ";";
    CHAR *token;

    token = strtok(outputBuffer, s);

    for (DWORD i = 0; token != NULL; i++)
    {
        strcpy(processNames[i], token);
        token = strtok(NULL, s);
        processPids[i] = atoi(token);
        token = strtok(NULL, s);
        nrOfProcesses++;
    }

    printf("Got %d nr of processes from HV\n", nrOfProcesses);

    CompareProcessList(processNames, processPids, nrOfProcesses);

    if (NULL != outputBuffer)
    {
        free(outputBuffer);
    }
    
    return 0;
}