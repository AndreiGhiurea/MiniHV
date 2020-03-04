#include "init_asm.h"
#include "idt.h"
#include "serial.h"
#include "display.h"
#include "pte.h"
#include "lock.h"
#include "apic.h"
#include "cpumu.h"
#include "vmcs.h"
#include "ept.h"
#include "segment.h"


// these are the functions linked from the assembly code
extern void ChangeStack();

extern void LoadGDT();
extern void LoadTR();

extern void VmPreLaunch();
extern void VmPreResume();

extern void InvalidateEPT();

INVALIDATE_EPT __inv_ept;

CHANGE_STACK __changeStack;

LOAD_GDT __loadGDT;
LOAD_TR __loadTR;

VM_PRE_LAUNCH __vm_preLaunch;
VM_PRE_RESUME __vm_preResume;

void
ImportAsmFunctions(
    void
)
{
    __changeStack = ( CHANGE_STACK ) ChangeStack;

    __loadGDT = ( LOAD_GDT ) LoadGDT;
    __loadTR = ( LOAD_TR ) LoadTR;

    __vm_preLaunch = ( VM_PRE_LAUNCH ) VmPreLaunch;
    __vm_preResume = ( VM_PRE_RESUME ) VmPreResume;

    __inv_ept = (INVALIDATE_EPT)InvalidateEPT;
}