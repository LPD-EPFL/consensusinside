#ifndef _PBFT_C_Modify_h
#define _PBFT_C_Modify_h 1
#include "PBFT_C_State_defs.h"

/* Needs to be kept in sync with PBFT_C_Bitmap.h */

#ifdef NO_STATE_TRANSLATION

extern unsigned long *_Byz_cow_bits;
extern char *PBFT_C_Byz_mem;
extern void _Byz_modify_index(int bindex);
static const int _LONGBITS = sizeof(long)*8;

#define _Byz_cow_bit(bindex) (_Byz_cow_bits[bindex/_LONGBITS] & (1UL << (bindex%_LONGBITS)))

#define _Byz_modify1(mem)  do {                                    \
  int bindex;                                                      \
  bindex = ((char*)(mem)-PBFT_C_Byz_mem)/Block_size;                     \
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
  bindex1 = (_mem-PBFT_C_Byz_mem)/Block_size;                            \
  bindex2 = (_mem+_size-1-PBFT_C_Byz_mem)/Block_size;                    \
  if ((_Byz_cow_bit(bindex1) == 0) | (_Byz_cow_bit(bindex2) == 0)) \
    Byz_modify(_mem,_size);                                        \
} while(0)

#endif

#endif /* _PBFT_C_Modify_h */
