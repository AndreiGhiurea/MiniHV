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
    BYTE* buff = "This string is from guest user-mode";

    printf("Buffer addr: 0x%x\n", &buff[0]);
    printf("Buffer: %s\n", buff);

    DWORD fs = __readfsdword(0x0);
    printf("FS:[0x0]: %x\n", fs);

    _VmcallToHV(sizeof(buff), buff, 0, NULL, NULL);

    return 0;
}