#ifndef _INIT_ASM_H_
#define _INIT_ASM_H_

#include "minihv.h"


//******************************************************************************
// Function:      ImportAsmFunctions
// Description: Links all the assembly functions. I.e. this function needs to be
//              called before any underscore named function.
// Returns:       void
// Parameter:     void
//******************************************************************************
void
ImportAsmFunctions(
    void
);


#endif // _INIT_ASM_H_