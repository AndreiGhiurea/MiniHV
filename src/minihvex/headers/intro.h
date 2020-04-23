#pragma once

#include "minihv.h"

STATUS
IntFindKernelBase(
    _In_ QWORD KernelAddr
);

STATUS
IntGetActiveProcessesList(
    _In_ DWORD BufferSize,
    _Inout_ CHAR* Buffer
);