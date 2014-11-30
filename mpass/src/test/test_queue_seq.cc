/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#include <stdio.h>

#include "../mpass/queue.h"

static void write_item_test(mpass::CacheLine *item, void *data);
static void read_item_test(mpass::CacheLine *item, void *data);

void test_queue_seq() {
	mpass::Queue<> *q = new(0,0) mpass::Queue<>();

	for(unsigned i = 0;i < 7;i++) {
		q->enqueue(write_item_test, (void *)(uintptr_t)i);
	}

	for(unsigned i = 0;i < 7;i++) {
		q->dequeue(read_item_test, NULL);
	}

	for(unsigned i = 7;i < 14;i++) {
		q->enqueue(write_item_test, (void *)(uintptr_t)i);
	}
	
	for(unsigned i = 7;i < 14;i++) {
		q->dequeue(read_item_test, NULL);
	}
}

void write_item_test(mpass::CacheLine *item, void *data) {
	uint32_t num = (uint32_t)(uintptr_t)(data);
	uint32_t *dest = (uint32_t *)item;
	*dest = num;
}

void read_item_test(mpass::CacheLine *item, void *data) {
	uint32_t *src = (uint32_t *)item;
	printf("Dequeued %u\n", *src);
}

