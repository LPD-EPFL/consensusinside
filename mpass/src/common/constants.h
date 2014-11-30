/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef MPASS_CONSTANTS_H_
#define MPASS_CONSTANTS_H_

#ifdef MPASS_X86
//#define CACHE_LINE_SIZE_BYTES 64
#define CACHE_LINE_SIZE_BYTES 128
#define LOG_CACHE_LINE_SIZE_BYTES 6
#elif defined MPASS_SPARC
//#define CACHE_LINE_SIZE_BYTES 64
#define CACHE_LINE_SIZE_BYTES 128
#define LOG_CACHE_LINE_SIZE_BYTES 6
#endif /* arch */

#define CACHE_LINE_ALIGNED __attribute__((aligned(CACHE_LINE_SIZE_BYTES)))

#include "word.h"

namespace mpass {

#ifdef MPASS_32
	const unsigned ADDRESS_SPACE_SIZE = 32;
	const unsigned LOG_BYTES_IN_WORD = 2;
#elif defined MPASS_64
	const unsigned ADDRESS_SPACE_SIZE = 64;
	const unsigned LOG_BYTES_IN_WORD = 3;
#endif /* X86_64 */

	const uintptr_t BYTES_IN_WORD = 1 << LOG_BYTES_IN_WORD;
	const uintptr_t WORD_ADDRESS_MASK = BYTES_IN_WORD - 1;

	inline Word *get_word_address(void *address) {
		return (Word *)((uintptr_t)address & ~WORD_ADDRESS_MASK);
	}

	inline unsigned get_byte_in_word_index(void *address) {
		return (uintptr_t)address & (uintptr_t)WORD_ADDRESS_MASK;
	}

	union word_to_bytes {
		uint8_t bytes[BYTES_IN_WORD];
		Word word;
	};

	//Added by Maysam Yabandeh
	//necessary for creating point-to-point channels
	const unsigned MAX_NUM_PROCESSES = 10;
	
}

#endif /* MPASS_CONSTANTS_H_ */
