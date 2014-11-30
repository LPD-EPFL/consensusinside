/**
 * This is a low-overhead extendible array. It was initially created to store
 * connection endpoints.
 *
 * Constructors for elements held in the array will not be invoked.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef MPASS_DYNAMIC_ARRAY_H_
#define MPASS_DYNAMIC_ARRAY_H_

#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "alloc.h"

namespace mpass {

	template<class T, unsigned INITIAL_CAPACITY = 64>
	class DynamicArray : public MpassAlloced {
		public:
            DynamicArray();
            //~DynamicArray();
            void init();
            void destroy();

            // checks for current capacity and size and extends
            // the array if necessary
            unsigned append();

            // access the array, but do not check array bounds
            T& operator[](int idx);

            unsigned get_size() const;

		protected:
			unsigned size;
			unsigned capacity;
			T *array;
	};
}

//Added by Maysam Yabandeh
template<class T, unsigned INITIAL_CAPACITY>
mpass::DynamicArray<T, INITIAL_CAPACITY>::DynamicArray() {
init();
}

template<class T, unsigned INITIAL_CAPACITY>
inline void mpass::DynamicArray<T, INITIAL_CAPACITY>::init() {
    size = 0;
    capacity = INITIAL_CAPACITY;
    array = (T *)malloc(sizeof(T) * INITIAL_CAPACITY);
}

template<class T, unsigned INITIAL_CAPACITY>
inline void mpass::DynamicArray<T, INITIAL_CAPACITY>::destroy() {
    free(array);
}


template<class T, unsigned INITIAL_CAPACITY>
inline unsigned mpass::DynamicArray<T, INITIAL_CAPACITY>::append() {
    // if the array is full, increase the size and copy the array
	 //fprintf(stderr, "append size=%d c=%d\n", size, capacity);
    if(size == capacity) {
        unsigned new_capacity = capacity * 2;
        T *new_array = (T *)malloc(sizeof(T) * new_capacity);
        memcpy(new_array, array, sizeof(T) * capacity);
        free(array);
        array = new_array;
        capacity = new_capacity;
    }

    return size++;
}

template<class T, unsigned INITIAL_CAPACITY>
inline T& mpass::DynamicArray<T, INITIAL_CAPACITY>::operator[](int idx) {
    return array[idx];
}

template<class T, unsigned INITIAL_CAPACITY>
inline unsigned mpass::DynamicArray<T, INITIAL_CAPACITY>::get_size() const {
    return size;
}


#endif /* MPASS_DYNAMIC_ARRAY_H_ */
