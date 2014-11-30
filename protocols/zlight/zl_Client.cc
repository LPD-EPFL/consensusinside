#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>

#include "Traces.h"
#include "th_assert.h"
#include "zl_Client.h"
#include "zl_Message.h"
#include "zl_Message_tags.h"
#include "zl_Reply.h"
#include "zl_Request.h"

// Force template instantiation
#include "zl_Certificate.t"
template class zl_Certificate<zl_Reply>;

zl_Client::zl_Client(FILE *config_file, char *host_name, short port) :
	zl_Node(config_file, NULL, host_name, port), r_reps(3*f()+1)
{
	// Fail if node is is a replica.
	if (is_replica(id()))
	{
		th_fail("Node is a replica");
	}

	out_rid = new_rid();
	out_req = 0;

	n_retrans = 0;
	rtimeout = 250;
	rtimer = new zl_ITimer(rtimeout, zl_rtimezl_handler);
}

zl_Client::~zl_Client()
{
    delete rtimer;
}

bool zl_Client::send_request(zl_Request *req, int size, bool ro)
{
	if (out_req != 0)
	{
		return false;
	}

#ifdef TRACES
	fprintf(
			stderr,
			"zl_Client::send_request (out_rid = %llu) (req->rid = %llu) (req->size = %d)\n",
			out_rid, req->request_id(), req->size());
#endif

	out_req = req;
	req->request_id() = out_rid;
	req->authenticate(size, ro);

	r_reps.clear();
	n_retrans = 0;
	send(req, primary());

	rtimer->start();

	return true;
}

zl_Reply *zl_Client::recv_reply()
{
#ifdef TRACES
	fprintf(stderr,"zl_Client:: In recv reply\n");
#endif

	if (out_req == 0)
	{
		// Nothing to wait for.
		fprintf(stderr, "zl_Client::recv_reply: no outgoing request\n");
		return 0;
	}

	// Wait for reply
	while (1)
	{
		zl_Message *m = zl_Node::recv();

		zl_Reply* rep;
		if (!zl_Reply::convert(m, rep))
		{
			delete m;
			continue;
		}
		if (rep->request_id() != out_rid)
		{
			delete rep;
			continue;
		}
		if (!r_reps.add(rep, zl_node))
		{
			delete rep;
			continue;
		}

		if (r_reps.is_complete())
		{
			zl_Reply *r = (r_reps.cvalue()->full()) ? r_reps.cvalue() : 0;

			if (r != 0)
			{
				// We got a reply
				// check to see whether we need to switch
				if (!r->should_switch()) {
				  out_rid = new_rid();
				}
				out_req = 0;
				rtimer->stop();
				r_reps.clear();
				return r;
			}
			fprintf(stderr,
			"zl_Client: Complete certificate without full reply...\n");
		}
		continue;
	}
}

void zl_rtimezl_handler()
{
	th_assert(zl_node, "zl_Client is not initialized");
	((zl_Client*) zl_node)->retransmit();
}

void zl_Client::retransmit()
{
	// Retransmit any outstanding request.
	static const int thresh = 1;
	static const int nk_thresh = 4;
	static const int nk_thresh_1 = 100;

	if (out_req != 0)
	{
		//    fprintf(stderr, ".");
		n_retrans++;
		if (n_retrans == nk_thresh || n_retrans % nk_thresh_1 == 0)
		{
		    rtimer->stop();
		    return;
		}

		bool ro = out_req->is_read_only();
		bool change = (ro || out_req->replier() >= 0) && n_retrans > thresh;
		//    printf("%d %d %d %d\n", id(), n_retrans, ro, out_req->replier());

#ifdef TRACES
		fprintf(stderr, "zl_Client[%d]: retransmitting the request\n", id());
#endif
		// read-only requests, requests retransmitted more than
		// mcast_threshold times, and big requests are multicast to all
		// zl_replicas.
		if (ro || n_retrans > thresh)
		    send(out_req, zl_All_replicas);
		else
		    send(out_req, primary());
	}

#ifdef ADJUST_RTIMEOUT
	// exponential back off
	if (rtimeout < Min_rtimeout) rtimeout = 100;
	rtimeout = rtimeout+lrand48()%rtimeout;
	if (rtimeout> Max_rtimeout) rtimeout = Max_rtimeout;
	rtimer->adjust(rtimeout);
#endif

	rtimer->restart();
}


