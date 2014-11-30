/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef MPASS_ALLOC_H_
#define MPASS_ALLOC_H_

#include <stdlib.h>

namespace mpass {

	class Alloc {
		public:
			static void *Malloc(size_t size) {
				return ::malloc(size);
			}

			static void Free(void *ptr) {
				::free(ptr);
			}
	};

	/**
	 * Use non-tx free/malloc provided by wlpdstm.
	 */
	class MpassAlloced {
	public:
		void* operator new(size_t size) {
			return Alloc::Malloc(size);
		}
		
		void* operator new[](size_t size) {
			return Alloc::Malloc(size);
		}
		
		void operator delete(void* ptr) {
			Alloc::Free(ptr);
		}
		
		void operator delete[](void *ptr) {
			return Alloc::Free(ptr);
		}		
	};	
}

#endif /* MPASS_ALLOC_H_ */
