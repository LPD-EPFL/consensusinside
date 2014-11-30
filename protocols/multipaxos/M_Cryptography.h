#ifndef _M_Cryptography_h
#define _M_Cryptography_h 1

#include "types.h"


// Sizes in bytes.
const unsigned int M_UMAC_size = 8;
const unsigned int M_UNonce_size = sizeof(long long);
const unsigned int M_MAC_size = M_UMAC_size + M_UNonce_size;

const unsigned int M_Nonce_size = 16;
const unsigned int M_Nonce_size_u = M_Nonce_size/sizeof(unsigned);
const unsigned int M_Key_size = 16;
const unsigned int M_Key_size_u = M_Key_size/sizeof(unsigned);

#endif // _M_Cryptography_h
