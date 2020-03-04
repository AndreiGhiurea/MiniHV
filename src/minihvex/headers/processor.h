#ifndef _PROCESSOR_H_
#define _PROCESSOR_H_

#define MAXPHYADDR                              52

#define MEMORY_TYPE_STRONG_UNCACHEABLE          0
#define MEMORY_TYPE_WRITECOMBINE                1
#define MEMORY_TYPE_WRITETHROUGH                4
#define MEMORY_TYPE_WRITEPROTECT                5
#define MEMORY_TYPE_WRITEBACK                   6
#define MEMORY_TYPE_UNCACHEABLE                 7

#define MEMORY_TYPE_MAXIMUM_MTRR                7       // MTRR only use memory types until MEMORY_TYPE_WRITEBACK

#endif // _PROCESSOR_H_