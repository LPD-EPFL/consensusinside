#ifndef _zl_Cryptography_h
#define _zl_Cryptography_h 1

#include "types.h"


// Sizes in bytes.
const unsigned int zl_UMAC_size = 8;
const unsigned int zl_UNonce_size = sizeof(long long);
const unsigned int zl_MAC_size = zl_UMAC_size + zl_UNonce_size;

const unsigned int zl_Nonce_size = 16;
const unsigned int zl_Nonce_size_u = zl_Nonce_size/sizeof(unsigned);
const unsigned int zl_Key_size = 16;
const unsigned int zl_Key_size_u = zl_Key_size/sizeof(unsigned);

#endif // _zl_Cryptography_h
