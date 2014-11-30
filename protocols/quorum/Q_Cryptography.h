#ifndef _Q_Cryptography_h
#define _Q_Cryptography_h 1

#include "types.h"


// Sizes in bytes.
const unsigned int Q_UMAC_size = 8;
const unsigned int Q_UNonce_size = sizeof(long long);
const unsigned int Q_MAC_size = Q_UMAC_size + Q_UNonce_size;

const unsigned int Q_Nonce_size = 16;
const unsigned int Q_Nonce_size_u = Q_Nonce_size/sizeof(unsigned);
const unsigned int Q_Key_size = 16;
const unsigned int Q_Key_size_u = Q_Key_size/sizeof(unsigned);

#endif // _Q_Cryptography_h
