#ifndef _DMP_VMCS_H_
#define _DMP_VMCS_H_

#include "minihv.h"
#include "vmcs.h"


void
DumpCurrentVmcs(
    IN_OPT      REGISTER_AREA*        ProcessorState
);


#endif // _DMP_VMCS_H_