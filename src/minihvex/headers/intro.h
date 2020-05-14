#pragma once

#include "minihv.h"

STATUS
IntFindKernelBase(
    _In_ QWORD KernelAddr
);

STATUS
IntGetActiveProcessesList(
    _In_ DWORD BufferSize,
    _Out_ CHAR* const Buffer,
    _Out_opt_ DWORD* const NrOfProcesses
);