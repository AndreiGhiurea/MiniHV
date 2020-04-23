#pragma once

#include "minihv.h"

STATUS
IntFindKernelBase(
    _In_ QWORD KernelAddr
);

STATUS 
IntGetActiveEprocess(DWORD* Pid
);