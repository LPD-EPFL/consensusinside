#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>

#include "th_assert.h"
#include "C_Client.h"
#include "C_Message.h"
#include "C_Reply.h"
#include "C_Request.h"

//#define TRACES

void*replies_handler_helper(void *o)
{
	void **o2 = (void **)o;
	C_Client &r = (C_Client&)(*o2);
	r.replies_handler();
	return 0;
}

C_Client::C_Client(FILE *config_file, char* host_name, short port) :
	C_Node(config_file, NULL, host_name, port)
{
	// Fail if C_node is is a replica.
	if (is_replica(id()))
	{
		th_fail("C_Node is a replica");
	}
	C_node=this;
	node=this;
	//  out_rid = new_rid();
	//out_req = 0;

	out_rid = new_rid();
	out_req = 0;
	nb_received_requests = 0;

	// Connect to primary (principals[0])
	fprintf(stderr, "C_Client: trying to connect to head %d:%d\n", principals[0]->TCP_addr_for_clients.sin_addr.s_addr, ntohs(principals[0]->TCP_addr_for_clients.sin_port));
	socket_to_primary = createClientSocket(principals[0]->TCP_addr_for_clients);
	/*   int flag = 1;
	 int result = setsockopt(socket_to_primary,IPPROTO_TCP,TCP_NODELAY,(char *)&flag,sizeof(int));
	 if(result<0){

	 fprintf(stderr, "Failed to disable Naggle for socket to primary in the chain\n");
	 exit(1);

	 }
	 */
	// Connect to last (principals[num_replicas - 1])
#ifndef REPLY_BY_PRIMARY
	socket_to_last
			= createClientSocket(principals[num_replicas - 1]->TCP_addr_for_clients);
#endif
	/* result = setsockopt(socket_to_last,IPPROTO_TCP,TCP_NODELAY,(char *)&flag,sizeof(int));
	 if(result<0){

	 fprintf(stderr, "Failed to disable Naggle for socket to last in the chain\n");
	 exit(1);

	 }
	 */
	if (pthread_create(&replies_handler_thread, 0, &replies_handler_helper,
			(void *)this) != 0)
	{
		fprintf(stderr, "Failed to create the thread for receiving messages from predecessor in the chain\n");
		exit(1);
	}
}

C_Client::~C_Client()
{
}

void C_Client::replies_handler()
{
#ifdef REPLY_BY_PRIMARY
	int socket_to_a_replica = socket_to_primary;
#else
	int socket_to_a_replica = socket_to_last;
#endif

	while (1)
	{
		C_Message* m = C_node->recv(socket_to_a_replica);
		// C_Reply* n;
		if (m->tag()==C_Reply_tag)
		{
			// Enqueue the message
			pthread_mutex_lock(&incoming_queue_mutex);
			{
				incoming_queue.append(m);
				pthread_cond_signal(&not_empty_incoming_queue_cond);
			}
			pthread_mutex_unlock(&incoming_queue_mutex);
		} else
		{
			delete m;
		}
	}

}

bool C_Client::send_request(C_Request *req, int size, bool ro)
{
	if (ro)
	{
		fprintf(stderr, "Read-only requests are currently not handled\n");
		return false;
	}

	if (out_req == 0)
	{
		// Send request to service
		// read-write requests are sent to the primary only.
#ifdef TRACES
		fprintf(stderr, "Client::send_request (size = %d)\n", req->size());
#endif

#ifdef TRACES
		fprintf(
				stderr,
				"************ INVOKING A NEW REQUEST ************ (out_rid = %llu) (req->rid = %llu) (req->size = %d)\n",
				out_rid, req->request_id(), req->size());
		fprintf(stderr,
				"Client::send_request (out_rid = %llu) (req->rid = %llu)\n",
				out_rid, req->request_id());
#endif

		out_req = req;
		req->request_id() = get_rid();
		int len = req->size();
		send_all(socket_to_primary, (char *) req->contents(), &len);
		return true;
	} else
	{
		// Another request is being processed.
		return false;
	}
}

C_Reply *C_Client::recv_reply()
{
	if (out_req == 0)
	{
		// Nothing to wait for.
		fprintf(stderr, "Client::recv_reply: no outgoing request\n");
		return 0;
	}

	//
	// Wait for reply
	//
	while (1)
	{
		//   C_Message* m = recv();
		C_Message *m;
		//  fprintf(stderr,".");
		pthread_mutex_lock(&incoming_queue_mutex);
		{
			while (incoming_queue.size() == 0)
			{

				pthread_cond_wait(&not_empty_incoming_queue_cond,
						&incoming_queue_mutex);
			}
			m = incoming_queue.remove();
		}
		pthread_mutex_unlock(&incoming_queue_mutex);

		C_Reply* rep;
		if (!C_Reply::convert(m, rep))
		{
			delete m;
			continue;
		}

		if (rep->request_id() != out_rid)
		{
#ifdef TRACES
			fprintf(stderr, "Client::recv_reply: rid is %llu different from out rid %llu \n",rep->request_id(), out_rid);
#endif
			delete rep;
			continue;
		}
		if (!rep->verify())
		{
			fprintf(stderr, "Client::recv_reply: verify returns FALSE\n");
			exit(1);
		}
		nb_received_requests++;

		out_rid = new_rid();
		out_req = 0;
		return rep;
	}
}
