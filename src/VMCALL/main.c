#include <windows.h>
#include <stdio.h>

VOID _cdecl _VmcallToHV(
    DWORD InputBufferSize,
    PVOID InputBuffer,
    DWORD OutputBufferSize,
    VOID* OutputBuffer,
    VOID* Context
);

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
    

    _VmcallToHV(0, NULL, sizeof(outputBuffer), outputBuffer, NULL);

    printf("Running Processes:\n");

    printf(outputBuffer);

    return 0;
}