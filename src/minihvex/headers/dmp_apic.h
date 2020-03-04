#ifndef _DMP_APIC_H_
#define _DMP_APIC_H_

#include "minihv.h"
#include "acpi.h"

void
DumpMadtTable(
    IN      ACPI_TABLE_HEADER*       TableHeader
);

#endif // _DMP_APIC_H_
