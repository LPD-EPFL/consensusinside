#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>

#include "Traces.h"
#include "th_assert.h"
#include "Q_Client.h"
#include "Q_Message.h"
#include "Q_Message_tags.h"
#include "Q_Reply.h"
#include "Q_Request.h"
#include "Q_Panic.h"

#include <task.h>

//#define clientdbg
//#define TRACES
// Force template instantiation
#include "Q_Certificate.t"
template class Q_Certificate<Q_Reply>;

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

//Added by Maysam Yabandeh
bool Q_Client::recievedOrder = false;
int Q_Client::order;
int Q_Client::lastRequestSize;

Q_Client::Q_Client(FILE *config_file, char *host_name, short port) :
	Q_Node(config_file, NULL, host_name, port), r_reps(num_replicas)
	//Added by Maysam Yabandeh
	//Q_Node(config_file, NULL, host_name, port), r_reps(3*f()+1)
{
//mainOfTaskScheduler(0,NULL);
	// Fail if node is is a replica.
	if (is_replica(id()))
	{
		th_fail("Node is a replica");
	}

	out_rid = new_rid();
	out_req = 0;

	is_panicking = false;
	bail_out = false;

	n_retrans = 0;
	//rtimeout = 15;
	//rtimer = new Q_ITimer(rtimeout, Q_rtimeQ_handler);
}

Q_Client::~Q_Client()
{
    //delete rtimer;
}

bool Q_Client::send_request(Q_Request *req, int size, bool ro)
{
	if (out_req != 0)
	{
		return false;
	}

//Added by Maysam Yabandeh
Q_Client::lastRequestSize = size;
#ifdef TRACES
	fprintf(
			stderr,
			"Q_Client::send_request (out_rid = %llu) (req->rid = %llu) (req->size = %d)\n",
			out_rid, req->request_id(), req->size());
#endif

	out_req = req;
	if (id()==5 && ro)
	    req->set_unused();
	req->request_id() = out_rid;
	//Added by Maysam Yabandeh
	req->setOrder(-1);

	req->authenticate(size, false);

	r_reps.clear();
	n_retrans = 0;
	is_panicking = false;
	//Added by Maysam Yabandeh
	send(req, primary());
	req->clear_unused();

#ifdef ADJUST_RTIMEOUT
	// Adjust timeout to reflect average latency
	//Added by Maysam Yabandeh
	//rtimeout=15;
	//rtimer->adjust(rtimeout);

	// Start timer to measure request latency
	//latency.reset();
	//latency.start();
#endif
	//rtimer->restop();
	//rtimer->start();

	return true;
}

//Added by Maysam Yabandeh
bool Q_Client::send_ordered_request(Q_Request *req, int size, bool ro)
{
//Added by Maysam Yabandeh
size = Q_Client::lastRequestSize;
	//Added by Maysam Yabandeh
	if (out_req == 0)
	{
	   fprintf(stderr, "Q_Client::send_ordered_request: out_req is null");
		return false;
	}

#ifdef TRACES
	fprintf(
			stderr,
			"Q_Client::send_request (out_rid = %llu) (req->rid = %llu) (req->size = %d)\n",
			out_rid, req->request_id(), req->size());
#endif

	////Added by Maysam Yabandeh
	//out_req = req;
	//if (id()==5 && ro)
	    //req->set_unused();
	//req->request_id() = out_rid;
	
	//Added by Maysam Yabandeh
	req->setOrder(Q_Client::order);
	req->authenticate(size, false);

	//r_reps.clear();
	//n_retrans = 0;
	//is_panicking = false;
	send(req, Q_All_replicas);
	//req->clear_unused();

#ifdef ADJUST_RTIMEOUT
	// Adjust timeout to reflect average latency
	//rtimer->adjust(rtimeout);
	//Added by Maysam Yabandeh
	//rtimeout=15;
	//rtimer->adjust(rtimeout);
#endif
	//rtimer->restop();
	//rtimer->start();

	return true;
}

Q_Reply *Q_Client::recv_reply()
{
//fprintf(stderr,"=Q_Client::recv_reply\n");
#ifdef TRACES
	fprintf(stderr,"Q_Client:: In recv reply\n");
#endif

	if (out_req == 0)
	{
		// Nothing to wait for.
		fprintf(stderr, "Q_Client::recv_reply: no outgoing request\n");
		return 0;
	}

	// Wait for reply
	while (1)
	{
		if (bail_out) {
		    bail_out = false;
		    is_panicking = false;
		    r_reps.clear();
		    out_req = 0;
				//Added by Maysam Yabandeh
				Q_Client::recievedOrder = false;//reset it
				for (int i = 0; i < num_replicas; i++)
				responders[i] = false;
				//I do not know why they need this non-sense obj, so I send a dummy one
			   Q_Reply *rep = new Q_Reply();
		    return rep;
		}
		if (r_reps.is_complete())
		{
			Q_Reply *r = (r_reps.cvalue()->full()) ? r_reps.cvalue() : 0;

			if (r != 0)
			{
				// We got a reply
				// check to see whether we need to switch
				//rtimer->stop();
				out_rid = new_rid();
				if (r->should_switch()) {
				    //fprintf(stderr, "Q_Client[%d]: Should switch\n", id());
				}
				out_req = 0;
				//Added by Maysam Yabandeh
				Q_Client::recievedOrder = false;//reset it
				for (int i = 0; i < num_replicas; i++)
				responders[i] = false;

				n_retrans = 0;
				r_reps.clear();
				#ifdef clientdbg
			   fprintf(stderr, "Client(%d) completed order %d\n", id(), order);
				#endif 
				return r;
			}
			fprintf(stderr,
			"Complete certificate without full reply...\n");
		}
		//Added by Maysam Yabandeh
		taskyield();
	}
}

void Q_Client::processReceivedMessage(Q_Message* m, int sock)
{
//fprintf(stderr,"=Q_Client::processReceivedMessage\n");
#ifdef TRACES
	fprintf(stderr,"Q_Client:: In recv reply\n");
#endif

//fprintf(stderr,"=Q_Client::processReceivedMessage 1\n");
	Q_Reply* rep;
	if (!Q_Reply::convert(m, rep))
	{
		delete m;
		return;
	}
//fprintf(stderr,"=Q_Client::processReceivedMessage from %d order=%d\n", rep->id(), rep->getOrder(), order);
	if (rep->request_id() != out_rid)
	{
		delete rep;
		return;
	}
//fprintf(stderr,"=Q_Client::processReceivedMessage 3\n");
	//Added by Maysam Yabandeh
	if (rep->id() != primary() && !Q_Client::recievedOrder)
		//I do not expect reply from others when 
		//still i have not recieved the order
	{
		fprintf(stderr,"!Error: unexpected message from rep.id()\n");
		delete rep;
		return;
	}
//fprintf(stderr,"=Q_Client::processReceivedMessage 4\n");

	if (!r_reps.add(rep, Q_node))
	{
		if (r_reps.cvalue() != 0
				&& !r_reps.cvalue()->match(rep)) {
			// This is the time to panic!
			//rtimer->stop();
			//fprintf(stderr, "Q_Client[%d]: mismatch on %lld to %lld\n", id(), r_reps.cvalue()->request_id(), rep->request_id());
			n_retrans = 0;
			is_panicking = true;
			r_reps.clear();
			out_rid = new_rid();

			Q_Panic qp(id(), out_rid);
			send(&qp, Q_All_replicas);
			//rtimer->start();
		}
		delete rep;
		return;
	}
//fprintf(stderr,"=Q_Client::processReceivedMessage 5\n");

	//Added by Maysam Yabandeh
	if (rep->id() == primary()) //the primary reply has the order
	{
		Q_Client::recievedOrder = true;
		Q_Client::order = rep->getOrder();
		//fprintf(stderr, "=Client: send_ordered_request: id=%d rep->id=%d rep->order=%d order=%d\n", id(), rep->id(), rep->getOrder(), Q_Client::order);
		send_ordered_request(out_req,0,false);
		//fprintf(stderr, "=Client: after send_ordered_request: id=%d rep->id=%d rep->order=%d order=%d\n", id(), rep->id(), rep->getOrder(), Q_Client::order);
		//fprintf(stderr, "=Client: after\n");
		return;
	}
	responders[rep->id()] = true;	
//fprintf(stderr,"=Q_Client::processReceivedMessage 6\n");
}

void Q_rtimeQ_handler()
{
	th_assert(Q_node, "Q_Client is not initialized");
	((Q_Client*) Q_node)->retransmit();
}

void Q_Client::retransmit()
{
return;//no need with tcp
	// Retransmit any outstanding request.
	//Added by Maysam Yabandeh
	static const int nk_thresh = 100;
	//static const int nk_thresh_p = 5;

	if (out_req != 0)
	{
		//    fprintf(stderr, ".");
		n_retrans++;
		if (n_retrans == nk_thresh)
		{
		    fprintf(stderr, "Q_Client[%d]: They are not responding!\n", id());
		    n_retrans = 0;

		    r_reps.clear();
		    out_req = 0;
		    out_rid = new_rid();
		    Q_Panic qp(id(), out_rid);
		    send(&qp, Q_All_replicas);
		    is_panicking = true;
		    //rtimer->restart();
		    return;
		}

		//Added by Maysam Yabandeh - resend only to primary
		if (!Q_Client::recievedOrder)
		{
			//fprintf(stderr, "Q_Client[%d]: retransmission of %d to primary!\n", id(), out_req->request_id());
			send(out_req, primary());
		}
		else
		{
			//fprintf(stderr, "Q_Client[%d]: retransmission of %d(%d)(%d) to replicas!\n", id(), out_req->request_id(),Q_Client::order, out_req->getOrder());
			for (int x=0; x<num_replicas; x++)
				if (x != primary())
					if (!responders[x])
						send(out_req, x);
		}
	} else if (is_panicking) {
		n_retrans++;
		if (n_retrans == nk_thresh)
		{
		    fprintf(stderr, "Q_Client[%d]: They are not responding on panic!\n", id());
		    //rtimer->stop();
		    n_retrans = 0;

		    // now, cause Q_Node to exit recv, and then to gracefully exit...
		    bail_out = true;
		    u_char yes = 1;
		    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &yes, sizeof(yes));
		    Digest d;
		    d.zero();
		    Q_Reply qr((View)0, out_rid, id(), d, i_to_p(id()), id());
		    qr.set_instance_id(pbft);
		    send (&qr, id());
		    u_char no = 0;
		    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &no, sizeof(no));

		    r_reps.clear();
		    is_panicking = false;
		    return;
		}
	}

#ifdef ADJUST_RTIMEOUT
	// exponential back off
	//if (rtimeout < Min_rtimeout) rtimeout = Min_rtimeout;
	//rtimeout = 2 * rtimeout+lrand48()%rtimeout;
	//if (rtimeout> Max_rtimeout) rtimeout = Max_rtimeout;
	//rtimer->adjust(rtimeout);
#endif

	//rtimer->restart();
}

