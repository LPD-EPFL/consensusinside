#ifndef _PBFT_R_Modify_h
#define _PBFT_R_Modify_h 1
#include "PBFT_R_State_defs.h"

/* Needs to be kept in sync with PBFT_R_Bitmap.h */

#ifdef NO_STATE_TRANSLATION

extern unsigned long *PBFT_R__Byz_cow_bits;
extern char *PBFT_R__Byz_mem;
extern void _Byz_modify_index(int bindex);
static const int PBFT_R__LONGBITS = sizeof(long)*8;

#define _Byz_cow_bit(bindex) (PBFT_R__Byz_cow_bits[bindex/PBFT_R__LONGBITS] & (1UL << (bindex%PBFT_R__LONGBITS)))

#define _Byz_modify1(mem)  do {                                    \
  int bindex;                                                      \
  bindex = ((char*)(mem)-PBFT_R__Byz_mem)/Block_size;                     \
  if (_Byz_cow_bit(bindex) == 0)                                   \
    _Byz_modify_index(bindex);                                     \
} while(0)

#define _Byz_modify2(mem,size) do {                                \
  int bindex1;                                                     \
  int bindex2;                                                     \
  char *_mem;                                                      \
  int _size;                                                       \
  _mem = (char*)(mem);                                             \
  _size = size;                                                    \
  bindex1 = (_mem-PBFT_R__Byz_mem)/Block_size;                            \
  bindex2 = (_mem+_size-1-PBFT_R__Byz_mem)/Block_size;                    \
  if ((_Byz_cow_bit(bindex1) == 0) | (_Byz_cow_bit(bindex2) == 0)) \
    Byz_modify(_mem,_size);                                        \
} while(0)

#endif

#endif /* _PBFT_R_Modify_h */
