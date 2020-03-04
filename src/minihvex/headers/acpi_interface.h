#ifndef _ACPI_INTERFACE_H_
#define _ACPI_INTERFACE_H_

#include "minihv.h"
#include "acpi.h"
#include "apic.h"


//******************************************************************************
// Function:      ApicRetrievePhysCpu
// Description: This function uses the ACPICA library to read the MADT ACPI
//              table to retrieve the list of CPUs.
// Returns:       void
// Parameter:     OUT LIST_ENTRY* PhysicalCpuList
//******************************************************************************
void
ApicRetrievePhysCpu(
    INOUT     LIST_ENTRY*     PhysicalCpuList
);

#endif // _ACPI_INTERFACE_H_