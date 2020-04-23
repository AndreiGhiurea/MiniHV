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
    CHAR outputBuffer[0x1000];

    _VmcallToHV(0, NULL, sizeof(outputBuffer), outputBuffer, NULL);

    printf("Running Processes:\n");

    printf(outputBuffer);

    return 0;
}