#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>

#include "Traces.h"
#include "th_assert.h"
#include "T_Client.h"
#include "T_Message.h"
#include "T_Message_tags.h"
#include "T_Reply.h"
#include "T_Request.h"

#include <task.h>

//#define clientdbg
//#define TRACES
// Force template instantiation
#include "T_Certificate.t"
template class T_Certificate<T_Reply>;

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

T_Client::T_Client(FILE *config_file, char *host_name, short port) :
	T_Node(config_file, NULL, host_name, port), r_reps(1) //only one reply is enough
{
	// Fail if node is is a replica.
	if (is_replica(id()))
	{
		th_fail("Node is a replica");
	}

   cur_rid = 0;
	out_rid = new_rid();
	out_req = 0;

	is_panicking = false;
}

T_Client::~T_Client()
{
}

bool T_Client::send_request(T_Request *req, int size, bool ro)
{
	if (out_req != 0)
	{
		return false;
	}

#ifdef TRACES
	fprintf(
			stderr,
			"T_Client::send_request (out_rid = %llu) (req->rid = %llu) (req->size = %d)\n",
			out_rid, req->request_id(), req->size());
#endif

	out_req = req;
	req->request_id() = out_rid;
	req->rep().cid = id();
	req->rep().sender_id = id();
	r_reps.clear();
	is_panicking = false;
	send(req, primary());
	return true;
}

T_Reply *T_Client::recv_reply()
{
#ifdef TRACES
	fprintf(stderr,"T_Client:: In recv reply\n");
#endif

	if (out_req == 0)
	{
		// Nothing to wait for.
		fprintf(stderr, "T_Client::recv_reply: no outgoing request\n");
		return 0;
	}

	// Wait for reply
	while (1)
	{
		if (r_reps.is_complete())
		{
			T_Reply *r = (r_reps.cvalue()->full()) ? r_reps.cvalue() : 0;

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

void T_Client::processReceivedMessage(T_Message* m, int sock)
{
#ifdef TRACES
	fprintf(stderr,"T_Client:: In recv reply\n");
#endif
	T_Reply* rep;
	if (!T_Reply::convert(m, rep))
	{
		delete m;
		return;
	}
	if (rep->request_id() != out_rid)
	{
		delete rep;
		return;
	}
	if (!r_reps.add(rep, T_node))
	{
		if (r_reps.cvalue() != 0
				&& !r_reps.cvalue()->match(rep)) {
			// This is the time to panic!
			fprintf(stderr, "T_Client[%d]: mismatch on %lld to %lld\n", id(), r_reps.cvalue()->request_id(), rep->request_id());
			is_panicking = true;
			r_reps.clear();
			exit(1);
		}
		delete rep;
		return;
	}
}

