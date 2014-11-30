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
#include "R_Client.h"
#include "R_Message.h"
#include "R_Reply.h"
#include "R_Request.h"

//#define TRACES

static void*replies_handler_helper(void *o)
{
	void **o2 = (void **)o;
	R_Client &r = (R_Client&)(*o2);
	r.replies_handler();
	return 0;
}

R_Client::R_Client(FILE *config_file, char* host_name, short port) :
	R_Node(config_file, NULL, host_name, port)
{
	// Fail if R_node is is a replica.
	if (is_replica(id()))
	{
		th_fail("R_Node is a replica");
	}
	R_node=this;
	node=this;
	//  out_rid = new_rid();
	//out_req = 0;

	out_rid = new_rid();
	out_req = 0;
	nb_received_requests = 0;

	// Connect to primary (principals[0])
	fprintf(stderr, "R_Client[%d]: trying to connect to %lx:%d as head\n", id(), principals[(id()%num_replicas)]->TCP_addr_for_clients.sin_addr.s_addr, principals[(id()%num_replicas)]->TCP_addr_for_clients.sin_port);
	socket_to_my_primary = createClientSocket(principals[((id()+0)%num_replicas)]->TCP_addr_for_clients);
	int cid = id();
	sendto(socket_to_my_primary, &cid, sizeof(cid), 0, 0, 0);
	/*   int flag = 1;
	 int result = setsockopt(socket_to_primary,IPPROTO_TCP,TCP_NODELAY,(char *)&flag,sizeof(int));
	 if(result<0){

	 fprintf(stderr, "Failed to disable Naggle for socket to primary in the ring\n");
	 exit(1);

	 }
	 */

#ifndef REPLY_BY_PRIMARY
	// Connect to last (principals[num_replicas - 1])
	fprintf(stderr, "R_Client[%d]: trying to connect to %lx:%d as tail\n", id(), principals[((id()+num_replicas-1)%num_replicas)]->TCP_addr_for_clients.sin_addr.s_addr, principals[((id()+num_replicas-1)%num_replicas)]->TCP_addr_for_clients.sin_port);
	socket_to_my_last
			= createClientSocket(principals[((id()+num_replicas-1)%num_replicas)]->TCP_addr_for_clients);
	sendto(socket_to_my_last, &cid, sizeof(cid), 0, 0, 0);
	/* result = setsockopt(socket_to_last,IPPROTO_TCP,TCP_NODELAY,(char *)&flag,sizeof(int));
	 if(result<0){

	 fprintf(stderr, "Failed to disable Naggle for socket to last in the ring\n");
	 exit(1);

	 }
	 */
#endif
	if (pthread_create(&replies_handler_thread, 0, &replies_handler_helper,
			(void *)this) != 0)
	{
		fprintf(stderr, "Failed to create the thread for receiving messages from predecessor in the ring\n");
		exit(1);
	}
}

R_Client::~R_Client()
{
}

void R_Client::replies_handler()
{
	fd_set socks;
	int readsocks;
	struct timeval timeout;
#ifdef REPLY_BY_PRIMARY
	int socket_to_a_replica = socket_to_my_primary;
#else
	int socket_to_a_replica = socket_to_my_last;
#endif

	while (1)
	{
		R_Message* m = R_node->recv(socket_to_a_replica);
		if (m == NULL) {
		    FD_ZERO(&socks);
		    FD_SET(socket_to_a_replica, &socks);
		    timeout.tv_sec = 1;
		    timeout.tv_usec = 0;
		    readsocks = select(socket_to_a_replica+1, &socks, (fd_set*)0, (fd_set*)0, &timeout);
		    if (readsocks < 0)
		    {
			fprintf(stderr, "select in requests_from_predecessor_handler");
			exit(1);
		    }
		    continue;
		}
		// R_Reply* n;
		if (m->tag()==R_Reply_tag)
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

bool R_Client::send_request(R_Request *req, int size, bool ro)
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
		int err = send_all(socket_to_my_primary, (char *) req->contents(), &len);
		return  err != -1;
	} else
	{
		// Another request is being processed.
		return false;
	}
}

R_Reply *R_Client::recv_reply()
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
		//   R_Message* m = recv();
		R_Message *m;
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

		R_Reply* rep;
		if (!R_Reply::convert(m, rep))
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
#ifdef USE_MACS
		if (!rep->verify())
		{
			fprintf(stderr, "Client::recv_reply: verify returns FALSE\n");
			exit(1);
		}
#endif
		nb_received_requests++;

		out_rid = new_rid();
		out_req = 0;
		return rep;
	}
}
