#ifndef _SUPERFASTHASH_
#define _SUPERFASTHASH_

uint32_t SuperFastHash (const char * data, int len);

template<class T>
struct SuperFastHasher{
   size_t operator()(const T& key_value) const {
	return SuperFastHash((const char *)&key_value, sizeof(T));
   }
};

#endif
