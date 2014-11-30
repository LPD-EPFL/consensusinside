#ifndef _R_Cryptography_h
#define _R_Cryptography_h 1

#include "types.h"


// Sizes in bytes.
const int R_UMAC_size = 8;
const int R_UNonce_size = sizeof(long long);
const int R_MAR_size = R_UMAC_size + R_UNonce_size;

const int R_Nonce_size = 16;
const int R_Nonce_size_u = R_Nonce_size/sizeof(unsigned);
const int R_Key_size = 16;
const int R_Key_size_u = R_Key_size/sizeof(unsigned);

#endif // _Cryptography_h
