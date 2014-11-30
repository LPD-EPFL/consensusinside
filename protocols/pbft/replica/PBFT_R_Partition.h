#ifndef _PBFT_R_Partition_h
#define _PBFT_R_Partition_h 1

#include "PBFT_R_pbft_libbyz.h"

//
// Definitions for hierarchical state partitions.
//

const int PChildren = 256;     // Number of children for non-leaf partitions.
const int PLevels = 4;       // Number of levels in partition tree.

// Number of siblings at each level.
const int PSize[] = {1, PChildren, PChildren, PChildren};

// Number of partitions at each level.
const int PLevelSize[] = {1, PChildren, PChildren*PChildren, PChildren*PChildren*PChildren};

// Number of blocks in a partition at each level
const int PBlocks[] = {PChildren*PChildren*PChildren, PChildren*PChildren, PChildren, 1};

#endif /* _PBFT_R_Partition_h */
