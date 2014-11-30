/**
 * Queue capacity should be a power of two minus one to implement modulo operation
 * more efficiently.
 *
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef MPASS_QUEUE_H_
#define MPASS_QUEUE_H_

#include <stdint.h>

#include "../common/constants.h"
#include "../common/word.h"
#include "../common/cache_aligned_alloc.h"
#include "../common/atomic.h"

//Added by Maysam Yabandeh
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

//#define maysam_dbg

namespace mpass {

	typedef uint8_t CacheLine[CACHE_LINE_SIZE_BYTES];

	typedef void (*WriteItemFunPtr)(CacheLine *item, void *data);
	typedef void (*ReadItemFunPtr)(CacheLine *item, void *data);

	template<unsigned CAPACITY = 7>
	class Queue : public CacheAlignedAlloc {
		public:
			Queue();

			void enqueue(WriteItemFunPtr write_item_fun, void *data);
			void dequeue(ReadItemFunPtr read_item_fun, void *data);

			bool is_empty();
			bool is_full();

			//Added by Maysam Yabandeh
			void* operator new(size_t, int &shmfd, int sender, int receiver);
			static void* createShmObj(size_t, int &shmfd, int sender, int receiver);
			void* operator new(size_t, int &shmfd, int listener);
			static void* createShmObj(size_t, int &shmfd, int listener);

		protected:
			static Word get_next_idx(Word idx);

		protected:
			CACHE_LINE_ALIGNED CacheLine data_items[CAPACITY + 1];

			// put both head and tail in the same cache line
			// TODO: check if it might be better to leave it in separate cache lines maybe
			CACHE_LINE_ALIGNED union {
				struct {
					volatile Word head;
					volatile Word tail;
				};
				char head_tail_padding[CACHE_LINE_SIZE_BYTES];
			};
	};

	typedef Queue<> QueueDefault;
}

template<unsigned CAPACITY>
inline mpass::Queue<CAPACITY>::Queue() : head(0), tail(0) {
	// empty
}

    struct simple_msg {
        simple_msg(void *d, size_t s) : data(d), size(s) {
            // do nothing
        }

        void *data;
        size_t size;
    };
template<unsigned CAPACITY>
inline void mpass::Queue<CAPACITY>::enqueue(WriteItemFunPtr write_item_fun, void *data) {
	Word new_tail = get_next_idx(tail);

	// while the queue is full
	while(new_tail == head) {
		// do nothing
	}

#ifdef maysam_dbg
   fprintf(stderr, "enqueue: data_items:%x head:%d tail:%d new_tail:%d \n", data_items, head, tail, new_tail);
#endif
	write_item_fun(data_items + new_tail, data);
	atomic_store_release(&tail, new_tail);
#ifdef maysam_dbg
   fprintf(stderr, "enqueue(after): data_items:%d head:%d tail:%d new_tail:%d \n", data_items, head, tail, new_tail);
#endif
}

template<unsigned CAPACITY>
inline void mpass::Queue<CAPACITY>::dequeue(ReadItemFunPtr read_item_fun, void *data) {
	// wait until there is something in the queue
	while(is_empty()) {
		// do nothing
	}

	Word new_head = get_next_idx(head);
#ifdef maysam_dbg
   fprintf(stderr, "dequeue: data_items:%x head:%d tail:%d new_head:%d \n", (int)(void*)data_items, head, tail, new_head);
#endif
	read_item_fun(data_items + new_head, data);
#ifdef maysam_dbg
   fprintf(stderr, "dequeue: after\n");
#endif
	atomic_store_release(&head, new_head);
#ifdef maysam_dbg
   fprintf(stderr, "dequeue: after\n");
#endif
}

template<unsigned CAPACITY>
inline bool mpass::Queue<CAPACITY>::is_empty() {
//static int cnt = 0;
//cnt++;
//if (cnt % 1000000 == 0)
   //fprintf(stderr, "empty: data_items:%x head:%d tail:%d \n", data_items, head, tail);
	return head == tail;
}

template<unsigned CAPACITY>
inline bool mpass::Queue<CAPACITY>::is_full() {
	return get_next_idx(tail) == head;
}

template<unsigned CAPACITY>
inline Word mpass::Queue<CAPACITY>::get_next_idx(Word idx) {
	return (idx + 1) & CAPACITY;
}

//Added by Maysam Yabandeh
//This function does not call the constructors. To do that call the new opeartor instead.
template<unsigned CAPACITY>
void* mpass::Queue<CAPACITY>::createShmObj(size_t shared_seg_size, int &shmfd, int sender, int receiver) {
	//int shmfd;
	/* Posix IPC object name [system dependant] */
	char SHMOBJ_PATH[256];
	sprintf(SHMOBJ_PATH, "/bft_%d_%d", sender, receiver);
	/* creating the shared memory object    --  shm_open()  */
	shmfd = shm_open(SHMOBJ_PATH, O_CREAT | O_EXCL | O_RDWR, S_IRWXU | S_IRWXG);
	if (shmfd < 0) {
		if (errno != EEXIST)
		{
			perror("In shm_open()");
			exit(1);
		}
		//this time it is OK if it already exists
		shmfd = shm_open(SHMOBJ_PATH, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);
		if (shmfd < 0)
		{
			perror("In shm_open()");
			exit(1);
		}
	}
	else { //only if it is just created
#ifdef maysam_dbg
		fprintf(stderr, "Created shared memory object %s\n", SHMOBJ_PATH);
#endif
		/* adjusting mapped file size (make room for the whole segment to map) */
		ftruncate(shmfd, shared_seg_size);
	}

	/* requesting the shared segment    --  mmap() */    
	void* pointer = mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	if (pointer == NULL) {
		perror("In mmap()");
		exit(1);
	}
#ifdef maysam_dbg
	fprintf(stderr, "Shared memory segment allocated correctly (%d bytes).\n", shared_seg_size);
#endif
	return pointer;
}

//Added by Maysam Yabandeh
//The object must be created in shared memory
template<unsigned CAPACITY>
void* mpass::Queue<CAPACITY>::operator new(size_t shared_seg_size, int &shmfd, int sender, int receiver) {
	printf("Queue new is called for sender=%d receiver=%d\n", sender, receiver);

	void* pointer = Queue<CAPACITY>::createShmObj(shared_seg_size, shmfd, sender, receiver);
	return pointer;
}

//Added by Maysam Yabandeh
//This function does not call the constructors. To do that call the new opeartor instead.
//This overloaded function creates a channel for listening
template<unsigned CAPACITY>
void* mpass::Queue<CAPACITY>::createShmObj(size_t shared_seg_size, int &shmfd, int listener) {
	//int shmfd;
	/* Posix IPC object name [system dependant] */
	char SHMOBJ_PATH[256];
	sprintf(SHMOBJ_PATH, "/bft_%d_listen", listener);
	/* creating the shared memory object    --  shm_open()  */
	shmfd = shm_open(SHMOBJ_PATH, O_CREAT | O_EXCL | O_RDWR, S_IRWXU | S_IRWXG);
	if (shmfd < 0) {
		if (errno != EEXIST)
		{
			perror("In shm_open()");
			exit(1);
		}
		//this time it is OK if it already exists
		shmfd = shm_open(SHMOBJ_PATH, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);
		if (shmfd < 0)
		{
			perror("In shm_open()");
			exit(1);
		}
	}
	else { //only if it is just created
#ifdef maysam_dbg
		fprintf(stderr, "Created shared memory object %s\n", SHMOBJ_PATH);
#endif
		/* adjusting mapped file size (make room for the whole segment to map) */
		ftruncate(shmfd, shared_seg_size);
	}

	/* requesting the shared segment    --  mmap() */    
	void* pointer = mmap(NULL, shared_seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
	if (pointer == NULL) {
		perror("In mmap()");
		exit(1);
	}
#ifdef maysam_dbg
	fprintf(stderr, "Shared memory segment allocated correctly (%d bytes).\n", shared_seg_size);
#endif
	return pointer;
}

//Added by Maysam Yabandeh
//The object must be created in shared memory
template<unsigned CAPACITY>
void* mpass::Queue<CAPACITY>::operator new(size_t shared_seg_size, int &shmfd, int listener) {
	printf("Queue new is called for listener=%d\n", listener);
	void* pointer = Queue<CAPACITY>::createShmObj(shared_seg_size, shmfd, listener);
	return pointer;
}

#endif /* MPASS_QUEUE_H_ */
