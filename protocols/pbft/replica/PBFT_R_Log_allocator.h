#ifndef _PBFT_R_Log_allocator_h
#define _PBFT_R_Log_allocator_h 1

#include <stdio.h>
#include <string.h>
#include "types.h"
#include "th_assert.h"


// Since messages may contain other messages in the payload. It is
// important to ensure proper alignment to allow access to the fields
// of embedded messages. The following macros are used to check and
// enforce alignment requirements. All message pointers and message
// sizes must satisfy ALIGNED.

// Minimum required alignment for correctly accessing message fields.
// Must be a power of 2.
#define ALIGNMENT 8

// bool ALIGNED(void *ptr) or bool ALIGNED(long sz)
// Effects: Returns true iff the argument is aligned to ALIGNMENT
#define ALIGNED(ptr) (((long)(ptr))%ALIGNMENT == 0)

// int ALIGNED_SIZE(int sz)
// Effects: Increases sz to the least multiple of ALIGNMENT greater
// than size.
#define ALIGNED_SIZE(sz) ((ALIGNED(sz)) ? (sz) : (sz)-(sz)%ALIGNMENT+ALIGNMENT)

#ifndef NDEBUG
#define DEBUG_ALLOC 1
#endif

#endif // _PBFT_R_Log_allocator_h
