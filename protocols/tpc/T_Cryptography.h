#ifndef _T_Cryptography_h
#define _T_Cryptography_h 1

#include "types.h"


// Sizes in bytes.
const unsigned int T_UMAC_size = 8;
const unsigned int T_UNonce_size = sizeof(long long);
const unsigned int T_MAC_size = T_UMAC_size + T_UNonce_size;

const unsigned int T_Nonce_size = 16;
const unsigned int T_Nonce_size_u = T_Nonce_size/sizeof(unsigned);
const unsigned int T_Key_size = 16;
const unsigned int T_Key_size_u = T_Key_size/sizeof(unsigned);

#endif // _T_Cryptography_h
