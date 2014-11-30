#ifndef _O_Cryptography_h
#define _O_Cryptography_h 1

#include "types.h"


// Sizes in bytes.
const unsigned int O_UMAC_size = 8;
const unsigned int O_UNonce_size = sizeof(long long);
const unsigned int O_MAC_size = O_UMAC_size + O_UNonce_size;

const unsigned int O_Nonce_size = 16;
const unsigned int O_Nonce_size_u = O_Nonce_size/sizeof(unsigned);
const unsigned int O_Key_size = 16;
const unsigned int O_Key_size_u = O_Key_size/sizeof(unsigned);

#endif // _O_Cryptography_h
