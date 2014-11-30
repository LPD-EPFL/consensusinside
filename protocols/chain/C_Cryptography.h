#ifndef _C_Cryptography_h
#define _C_Cryptography_h 1

#include "types.h"


// Sizes in bytes.
const int C_UMAC_size = 8;
const int C_UNonce_size = sizeof(long long);
const int C_MAC_size = C_UMAC_size + C_UNonce_size;

const int C_Nonce_size = 16;
const int C_Nonce_size_u = C_Nonce_size/sizeof(unsigned);
const int C_Key_size = 16;
const int C_Key_size_u = C_Key_size/sizeof(unsigned);

#endif // _Cryptography_h
