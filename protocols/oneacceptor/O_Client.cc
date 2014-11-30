#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>

#include "Traces.h"
#include "th_assert.h"
#include "O_Client.h"
#include "O_Message.h"
#include "O_Message_tags.h"
#include "O_Reply.h"
#include "O_Request.h"

#include <task.h>

//#define clientdbg
//#define TRACES
// Force template instantiation
#include "O_Certificate.t"
template class O_Certificate<O_Reply>;

#define printDebug(...) \
    do { \
	struct timeval tv; \
	gettimeofday(&tv, NULL); \
	fprintf(stderr, "%u.%06u: ", tv.tv_sec, tv.tv_usec); \
	fprintf(stderr, __VA_ARGS__); \
    } while (0);
#undef printDebug
#define printDebug(...) \
    do { } while(0);

void O_client_timer(void* v) {
   O_Client* clnt = (O_Client*)v;
	int threshhold=100;//ms
	while (1) { 
		Request_id prevId = clnt->get_out_rid();
		//taskdelay(10000);//10s
		taskdelay(threshhold);//1s
		if (prevId == clnt->get_out_rid()) //no change
		{
		    threshhold=10000;
		    int total = clnt->get_num_replicas();
			 clnt->set_currentLeader( (clnt->get_currentLeader() + 2) % total );//the next one is the acceptor TODO make it general
			fprintf(stderr, "--------- Client: Leader change to %d\n", clnt->get_currentLeader());
			if (clnt->get_out_req() != NULL)
				clnt->send(clnt->get_out_req(), clnt->get_currentLeader());
		}
	}
}

O_Client::O_Client(FILE *config_file, char *host_name, short port) :
	O_Node(config_file, NULL, host_name, port), r_reps(1) //only one reply is enough
{
	// Fail if node is is a replica.
	if (is_replica(id()))
	{
		th_fail("Node is a replica");
	}

   cur_rid = 0;
	out_rid = new_rid();
	out_req = 0;

	currentLeader = primary();

	is_panicking = false;

	taskcreate(O_client_timer, (void*)this, O_STACK);
}

O_Client::~O_Client()
{
}

bool O_Client::send_request(O_Request *req, int size, bool ro)
{
	if (out_req != 0)
	{
		return false;
	}

#ifdef TRACES
	fprintf(
			stderr,
			"O_Client::send_request (out_rid = %llu) (req->rid = %llu) (req->size = %d)\n",
			out_rid, req->request_id(), req->size());
#endif

	out_req = req;
	req->request_id() = out_rid;
	req->client_id() = id();
	req->sender_id() = id();
	r_reps.clear();
	is_panicking = false;
	send(req, currentLeader);
	return true;
}

O_Reply *O_Client::recv_reply()
{
#ifdef TRACES
	fprintf(stderr,"O_Client:: In recv reply\n");
#endif

	if (out_req == 0)
	{
		// Nothing to wait for.
		fprintf(stderr, "O_Client::recv_reply: no outgoing request\n");
		return 0;
	}

	// Wait for reply
	while (1)
	{
		if (r_reps.is_complete())
		{
			O_Reply *r = (r_reps.cvalue()->full()) ? r_reps.cvalue() : 0;

			if (r != 0)
			{
				out_rid = new_rid();
				out_req = 0;
				r_reps.clear();
				#ifdef clientdbg
			   fprintf(stderr, "Client(%d) completed \n", id());
				#endif 
				return r;
			}
			fprintf(stderr, "Complete certificate without full reply...\n");
		}
		taskyield();
	}
}

void O_Client::handle(O_Abandon *m, int sock)
{
	th_assert(out_req != NULL, "receive abandon but i have no out request");
	fprintf(stderr, "--------- Client %d: Leader change from %d to %d\n", id(), currentLeader, m->leader_id());
	currentLeader = m->leader_id();
	send(out_req, currentLeader);
}

void O_Client::processReceivedMessage(O_Message* m, int sock)
{
#ifdef TRACES
	fprintf(stderr,"O_Client:: In recv reply\n");
#endif
	if (m->tag() == O_Abandon_tag)
	{
		O_Abandon *aba;
		if (!O_Abandon::convert(m, aba))
			th_assert(0, "Error in converting the Abandon message");
		handle(aba, sock);
		return;
	}
	O_Reply* rep;
	if (!O_Reply::convert(m, rep))
	{
		delete m;
		return;
	}
	if (rep->request_id() != out_rid)
	{
		delete rep;
		return;
	}
	if (!r_reps.add(rep, O_node))
	{
		if (r_reps.cvalue() != 0
				&& !r_reps.cvalue()->match(rep)) {
			// This is the time to panic!
			fprintf(stderr, "O_Client[%d]: mismatch on %lld to %lld\n", id(), r_reps.cvalue()->request_id(), rep->request_id());
			is_panicking = true;
			r_reps.clear();
			exit(1);
		}
		delete rep;
		return;
	}
}

