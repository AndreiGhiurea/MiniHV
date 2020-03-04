#ifndef _MINIHV_H_
#define _MINIHV_H_

#include "common_lib.h"
#include "processor.h"
#include "msr.h"
#include "register.h"
#include "list.h"
#include "hv_assert.h"

#ifdef DEBUG
//#define TEST
//#define MULTI_TEST
#endif

// an 8KB stack size is more than enough for each CPU
#define EFFECTIVE_STACK_SIZE                (8*KB_SIZE)

// we allocate EFFECTIVE_STACK_SIZE + PAGE_SIZE so we can use a page guard
// i.e. we unmap the VA corresponding to the first PAGE_SIZE allocated
//      RSP --------->        4K region
//          --------->        4K region
//          --------->      UNMAPPED PAGE     <---------
//          --------->  OTHER IMPORTANT DATA
// So, if we get into an infinite recursion we will actual get a #PF and
// debugging will be possible
#define STACK_SIZE                          (EFFECTIVE_STACK_SIZE+PAGE_SIZE)

// size required by the MiniHV besides the code
#define MINIHV_ADDITIONAL_SIZE              0x08000000UL                                // 128 MB
#define OFFSET_TO_INITIAL_STACK_BASE        0x8000

// Virtual to Physical address conversion and vice-versa
#define VA2PA(addr)                         ((QWORD)(addr)-TB_SIZE)
#define PA2VA(addr)                         ((QWORD)(addr)+TB_SIZE)

#define INITIAL_BOOT_DISK_DRIVE             0x80

#define DATA64_SEL                          0x8
#define CODE64_SEL                          0x10

#endif // _MINIHV_H_