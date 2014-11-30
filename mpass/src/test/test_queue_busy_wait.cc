/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#include <stdio.h>
#include <pthread.h>

#include "../common/time.h"
#include "../common/inttypes.h"
#include "../mpass/queue.h"
#include "test_queue_busy_wait.h"

struct thread_data {
	mpass::QueueDefault *server_q;
	mpass::QueueDefault *client_q;
};

static void *thread_fun_server(void *data);
static void *thread_fun_client(void *data);

static void send_to_server(mpass::CacheLine *item, void *data);
static void server_receive(mpass::CacheLine *item, void *data);

static void send_to_client(mpass::CacheLine *item, void *data);
static void client_receive(mpass::CacheLine *item, void *data);

#define TEST_MESSAGE_COUNT 10000000
#define END_MESSAGE        0

static pthread_t server_t;
static volatile bool start_flag;

void test_queue_busy_wait() {
	thread_data queues;
	queues.server_q = new(0,0) mpass::QueueDefault();
	queues.client_q = new(1,1) mpass::QueueDefault();

	start_flag = false;

	pthread_create(&server_t, NULL, thread_fun_server, &queues);

	while(!start_flag) {
		// do nothing
	}

	thread_fun_client(&queues);
}

void *thread_fun_client(void *data) {
	thread_data *queues = (thread_data *)data;

	////Added by Maysam Yabandeh
	//uint64_t start_time = mpass::get_time_ns();
	uint64_t start_time = 0;

	for(int i = 1;i <= TEST_MESSAGE_COUNT;i++) {
		queues->server_q->enqueue(send_to_server, (void *)(uintptr_t)i);
		queues->client_q->dequeue(client_receive, NULL);
	}

	//Added by Maysam Yabandeh
	//uint64_t end_time = mpass::get_time_ns();
	uint64_t end_time = 0;

	queues->server_q->enqueue(send_to_server, (void *)(uintptr_t)0);

	printf("[busy_wait] Time to send %u messages and acks is: %" PRIu64 "\n",
		   TEST_MESSAGE_COUNT, end_time - start_time);

	return NULL;
}

void send_to_server(mpass::CacheLine *item, void *data) {
	uint32_t num = (uint32_t)(uintptr_t)(data);
	uint32_t *dest = (uint32_t *)item;
	*dest = num;	
}

void client_receive(mpass::CacheLine *item, void *data) {
	// do nothing
}

void *thread_fun_server(void *data) {
	thread_data *queues = (thread_data *)data;
	bool end_flag = false;

	start_flag = true;

	while(!end_flag) {
		queues->server_q->dequeue(server_receive, (void *)&end_flag);
		queues->client_q->enqueue(send_to_client, NULL);
	}

	return NULL;
}

void send_to_client(mpass::CacheLine *item, void *data) {
	// do nothing
}

void server_receive(mpass::CacheLine *item, void *data) {
	uint32_t *src = (uint32_t *)item;

	if(*src == END_MESSAGE) {
		bool *end_flag = (bool *)data;
		*end_flag = true;
	}
}
