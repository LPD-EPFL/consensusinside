#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/tcp.h>

#include <queue>

#include "th_assert.h"
#include "R_Message_tags.h"
#include "R_Request.h"
#include "R_ACK.h"
#include "R_Checkpoint.h"
#include "R_Reply.h"
#include "R_Principal.h"
#include "R_Replica.h"
#include "types.h"
#include "MD5.h"

#define ROUND1 1
#define ROUND2 2
#define ROUND3 3

#ifdef REPLY_BY_PRIMARY
#include "R_Deliver.h"
#endif

// #define TRACES

// Global replica object.
R_Replica *R_replica;

#define RANSWER (num_replicas - 1)

// Function for the thread receiving messages from the predecessor in the ring
//extern "C"
static void*requests_from_predecessor_handler_helper(void *o)
{
	void **o2 = (void **)o;
	R_Replica &r = (R_Replica&)(*o2);
	r.requests_from_predecessor_handler();
	return 0;
}

// Function for the thread receiving messages from clients
//extern "C"
void*R_client_requests_handler_helper(void *o)
{
	void **o2 = (void **)o;
	R_Replica &r = (R_Replica&)(*o2);
	r.c_client_requests_handler();
	return 0;
}

inline void*R_message_queue_handler_helper(void *o)
{
	void **o2 = (void **)o;
	R_Replica &r = (R_Replica&) (*o2);
	//temp_replica_class = (Replica<class Request_T, class Reply_T>&)(*o);
	//  r.recv1();
	//temp_replica_class.do_recv_from_queue();
	r.do_recv_from_queue();
	return 0;
}

R_Replica::R_Replica(FILE *config_file, FILE *config_priv, char* host_name, short port) :
	R_Node(config_file, config_priv, host_name, port), seqno(0), last_executed(0), replies(num_principals),
	incoming_queue_clients(),
	connectmap(50),
	vector_clock((Seqno)0, num_replicas),
	requests()
{
	// Fail if node is not a replica.
	if (!is_replica(id()))
	{
		th_fail("Node is not a replica");
	}

	// Read view change, status, and recovery timeouts from replica's portion
	// of "config_file"
	int vt, st, rt;
	fscanf(config_file, "%d\n", &vt);
	fscanf(config_file, "%d\n", &st);
	fscanf(config_file, "%d\n", &rt);

	// Create timers and randomize times to avoid collisions.
	srand48(getpid());

	exec_command = 0;

	connectmap.set_empty_key(-1);
	connectmap.set_deleted_key(-3);
	incoming_queue_signaler[0] = 0;
	incoming_queue_signaler[1] = 0;

	// Create server socket
	in_socket= createServerSocket(ntohs(principals[node_id]->TCP_addr.sin_port));

	// Create receiving thread
	if (pthread_create(&requests_from_predecessor_handler_thread, 0,
			&requests_from_predecessor_handler_helper, (void *)this) != 0)
	{
		fprintf(stderr, "Failed to create the thread for receiving messages from predecessor in the ring\n");
		exit(1);
	}

	// Connect to principals[(node_id + 1) % num_r]
	fprintf(stderr, "R_Replica[%d]: Trying to get out_socket\n", node_id);
	out_socket = createClientSocket(principals[(node_id + 1) % num_replicas]->TCP_addr);

	fprintf(stderr,"Creating client socket\n");
	in_socket_for_clients= createNonBlockingServerSocket(ntohs(principals[node_id]->TCP_addr_for_clients.sin_port));
	if (pthread_create(&R_client_requests_handler_thread, NULL,
		    &R_client_requests_handler_helper, (void *)this)!= 0)
	{
	    fprintf(stderr, "Failed to create the thread for receiving client requests\n");
	    exit(1);
	}
}

R_Replica::~R_Replica()
{
}

void R_Replica::do_recv_from_queue()
{
	R_Message *m;
	static int selector = 0;
	while (1)
	{
		m = NULL;
		pthread_mutex_lock(&incoming_queue_mutex);
		{
			while (incoming_queue.size() + incoming_queue_clients.size() == 0)
			{
				pthread_cond_wait(&not_empty_incoming_queue_cond, &incoming_queue_mutex);
			}
			// at this point, we know there is something in either queue
			ssize_t qs;
			if (selector == 0) {
			    qs = incoming_queue.size();
			} else {
			    qs = incoming_queue_clients.size();
			}
			if (!incoming_queue_signaler[selector] && qs == 0)
			    selector = 1-selector;
			// 0 is for predecessor,
			// 1 is for clients
			if (selector == 0) {
			    m = incoming_queue.remove();
			} else {
			    m = incoming_queue_clients.remove();
			}
			incoming_queue_signaler[selector] = 0;
			selector = 1-selector;
		}
		pthread_mutex_unlock(&incoming_queue_mutex);
		if (m != NULL)
		    handle(m);
	}
}

void R_Replica::register_exec(int(*e)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool))
{
	exec_command = e;
}

void R_Replica::requests_from_predecessor_handler()
{
	int socket = -1;

	listen(in_socket, 1);
	socket = accept(in_socket, NULL, NULL);
	if (socket < 0)
	{
		perror("Cannot accept connection \n");
		exit(-1);
	}

	fd_set socks;
	int readsocks;
	struct timeval timeout;
	// Loop to receive messages.
	while (1)
	{

		R_Message* m = R_Node::recv(socket);
		if (m == NULL) {
		    FD_ZERO(&socks);
		    FD_SET(socket, &socks);
		    timeout.tv_sec = 1;
		    timeout.tv_usec = 0;
		    readsocks = select(socket+1, &socks, (fd_set*)0, (fd_set*)0, &timeout);
		    if (readsocks < 0)
		    {
			perror("select in requests_from_predecessor_handler");
			exit(1);
		    }
		    continue;
		}
#ifdef TRACES
		fprintf(stderr, "Received message\n");
#endif
			// Enqueue the message
			pthread_mutex_lock(&incoming_queue_mutex);
			{
				incoming_queue.append(m);
				incoming_queue_signaler[0] = 1;
				pthread_cond_signal(&not_empty_incoming_queue_cond);
			}
			pthread_mutex_unlock(&incoming_queue_mutex);
	}
}

void R_Replica::handle(R_Message *m)
{
	// TODO: This should probably be a jump table.
	switch (m->tag())
	{
		case R_Request_tag:
			gen_handle<R_Request>(m);
			break;

		case R_ACK_tag:
			gen_handle<R_ACK>(m);
			break;

#ifdef REPLY_BY_PRIMARY
		case R_Deliver_tag:
			gen_handle<R_Deliver>(m);
			break;
#endif

		case R_Checkpoint_tag:
			gen_handle<R_Checkpoint>(m);
			break;

		default:
			// Unknown message type.
			delete m;
	}
}

void R_Replica::handle(R_Request *req)
{
	bool ro = req->is_read_only();
	int cid = req->client_id();

#ifdef TRACES
	fprintf(stderr, "*********** R_Replica %d: Receiving request to handle, from %d\n", id(), cid);
#endif

	if (ro)
	{
		fprintf(stderr, "R_Replica %d: Read-only requests are currently not handled\n", id());
		delete req;
		exit(1);
	}

	int authentication_offset;
#ifdef USE_MACS
	if (!req->verify(&authentication_offset, ROUND1))
	{
		fprintf(stderr, "R_Replica[%d]: verify returned FALSE\n", id());
		delete req;
		return;
	}
#endif
#ifdef TRACES
	fprintf(stderr, "*********** R_Replica %d: message verified\n", id());
#endif

	if (0 == id()) {
	    // should set both seqno, and sequenced
	    seqno++;
	    req->set_seqno(seqno);
	}

	Execution exr = execution_wait;
	if (req->is_sequenced()) {
		R_Request::Iterator rit = req->begin();
		while (rit != req->end()) {
		    //fprintf(stderr, "Replica %d: WILL EXECUTE REQ %p, at %d, pos %d\n", id(), rit.data.req, rit.data.offset, rit.data.index);
		    if (rit.data.req == rit.data.oreq)
			exr = execute_request(rit.data.req);
		    else
			execute_request_main(rit.data.req);
		    ++rit;
		}
	}

	// time to get other requests
	// and only if the request came directly from the client.
	if (req->replica_id() == id()) {
	    int max_count = 10;
	    int cur_size = req->size();
	    int orig_req_size = req->size();
	    std::queue<R_Request *> tobatch;
	    pthread_mutex_lock(&incoming_queue_mutex);
	    while (incoming_queue_clients.size() > 0 && max_count > 0)
	    {
		//fprintf(stderr, "R_Request[%d]: will check one request\n", id());
		if (incoming_queue_clients.first()->tag() != R_Request_tag)
		    break;
		if (incoming_queue_clients.first()->size() + cur_size >= R_Max_message_size)
		    break;
		R_Message *m = incoming_queue_clients.remove();
		R_Request *breq;
		if (!R_Request::convert(m, breq))
		{
		    delete m;
		    continue;
		}
		// to increase the speed, we will not verify now...
		cur_size += breq->size();
		max_count--;
		tobatch.push(breq);
	    }
	    pthread_mutex_unlock(&incoming_queue_mutex);
	    if (cur_size != req->size()) {
		R_Request *nreq = new R_Request(cur_size);
		memcpy(nreq->contents(), req->contents(), req->size());
		delete req;
		req = nreq;
	    }
	    while (tobatch.size() > 0)
	    {
		R_Request *breq;
		breq = tobatch.front();

		//fprintf(stderr, "\t\tsize %d, ptr %p, client %d\n", breq->size(), breq->contents(), breq->client_id());
		int auth_off;
		if (!breq->verify(&auth_off, ROUND1))
		{
		    fprintf(stderr, "R_Replica[%d]: verify of request for batching returned false\n", id());
		    delete breq;
		    continue;
		}
		breq->set_seqno(req->seqno());
		if (exr == execution_ok) {
		    execute_request_main(breq);
		}
		memcpy((char*)(req->contents()+orig_req_size), breq->contents(), breq->size());
		orig_req_size += ALIGNED_SIZE(breq->size());
		req->set_batched();
		delete breq;
		tobatch.pop();
	    }
	    req->set_size(orig_req_size);
	}
	//fprintf(stderr, "Msg size is now %d\n",req->size());

	R_Message_State pending_req;
	pending_req.req = req;
	pending_req.state = (req->is_sequenced() && exr != execution_wait)?state_exec_not_acked:state_pending;
	pending_req.authentication_offset = authentication_offset;
	R_Message_Id rmid(cid, req->request_id());
	requests[rmid] = pending_req;

	if (distance(req->replica_id(), id()) == RANSWER) {
	    R_ACK ack(cid, req->seqno(), req->request_id(), req->digest());
	    req->set_type(true);
#ifdef USE_MACS
	    req->authenticate(id(), authentication_offset, NULL, ROUND1);
	    req->copy_authenticators(ack.contents()+sizeof(R_ACK_rep));
#endif
	    // now, glue other acks for batched requests...
	    if (req->is_batched())
	    {
		int orig_req_size = ack.size();
		R_Request::Iterator rit = req->begin();
		while (rit != req->end()) {
		    if (rit.data.req != rit.data.oreq)
		    {
			R_ACK *tack = new R_ACK(rit.data.req->client_id(), rit.data.req->seqno(), rit.data.req->request_id(), rit.data.req->digest());
			memcpy((char*)ack.contents()+orig_req_size, tack->contents(), tack->size());
			orig_req_size += ALIGNED_SIZE(tack->size());
			delete tack;
		    }
		    ++rit;
		}
		ack.set_size(orig_req_size);
		ack.set_batched();
	    }
	    // and send it...
	    int len = ack.size();
	    send_all(out_socket, ack.contents(), &len);
#ifdef TRACES
	    fprintf(stderr, "R_Replica %d: Sending ACK of size %d\n", id(), len);
#endif
	} else {
		// Forwarding the request
#ifdef USE_MACS
		req->authenticate(id(), authentication_offset, NULL, ROUND1);
#endif
		int len = req->size();
		send_all(out_socket, req->contents(), &len);
		//fprintf(stderr, "R_Replica %d: Forwarding the request of size %d\n", id(), len);
		//return;
	}

	return;
}

Execution R_Replica::execute_request(R_Request *req)
{
	// check if request has been executed
	if (req->seqno() <= last_executed)
	    return execution_old;

	if (req->seqno() != last_executed + 1)
	    return execution_wait;

	execute_request_main(req);

	last_executed = req->seqno();
	return execution_ok;
}

void R_Replica::execute_request_main(R_Request *req)
{
	int cid = req->client_id();
	// Execute the request
	Byz_req inb;
	Byz_rep outb;
	inb.contents = req->command(inb.size);
	outb.contents = replies.new_reply(cid, outb.size);

	// Execute command in a regular request.
	exec_command(&inb, &outb, NULL, cid, false);

#ifdef TRACES
	fprintf(stderr, "*********** R_Replica %d: commmand executed\n", id());
#endif
	if (outb.size % ALIGNMENT_BYTES)
	{
		for (int i=0; i < ALIGNMENT_BYTES - (outb.size % ALIGNMENT_BYTES); i++)
		{
			outb.contents[outb.size+i] = 0;
		}
	}

	// Finish constructing the reply.
	R_Reply_rep *rr = replies.end_reply(cid, req->replica_id(), req->request_id(), req->seqno(), outb.size);
}


void R_Replica::send_reply_to_client(R_Request *req)
{
	int cid = req->client_id();
	// R_Replying to the client
#ifdef TRACES
	fprintf(stderr, "R_Replica[%d]: will reply to %d on socket %d for seqno %lld\n", id(), cid, connectmap[cid], req->seqno());
#endif
	replies.send_reply(cid, connectmap[cid], req->MACs());
}

void R_Replica::handle(R_ACK *ack)
{
	int cid = ack->client_id();
	Digest dig;

#ifdef TRACES
	fprintf(stderr, "*********** R_Replica %d: Receiving ACK(%d) to handle\n", id(), cid);
#endif

	R_Message_Id rmid(cid, ack->request_id());

	R_Message_Map::iterator it;
	it = requests.find(rmid);
	if (it == requests.end()) {
	    fprintf(stderr, "R_Replica[%d]: request not found for <%d,%lld>\n", id(), cid, ack->request_id());
	    delete ack;
	    return;
	}

	R_Request *areq = NULL;
	R_Message_State& rms = it->second;
	areq = rms.req;

#ifdef USE_MACS
	areq->replace_authenticators(ack->contents()+sizeof(R_ACK_rep));
#endif
	areq->set_type(true);
	areq->set_seqno(ack->seqno());

#ifdef USE_MACS
	int authentication_offset;
	if (!areq->verify(&authentication_offset, ROUND2)) {
		fprintf(stderr, "R_Replica %d: verify of ACK returned FALSE\n", id());
		delete ack;
		delete areq;
		requests.erase(it);
		return;
	}
	rms.authentication_offset = authentication_offset;
#endif

	if (rms.state == state_pending) {
		Execution exr;
		R_Request::Iterator rit = rms.req->begin();
		while (rit != rms.req->end()) {
		    //fprintf(stderr, "WILL EXECUTE BATCHED REQ %p, at %d, pos %d\n", rit.data.req, rit.data.offset, rit.data.index);
		    if (rit.data.req == rit.data.oreq)
			exr = execute_request(rit.data.req);
		    else {
			rit.data.req->set_type(true);
			rit.data.req->set_seqno(areq->seqno());
			execute_request_main(rit.data.req);
		    }
		    ++rit;
		}
	    if (exr == execution_wait) {
		fprintf(stderr, "R_Replica[%d]: There is a problem, request could not be executed (last_executed = %lld, seqno = %lld)\n", id(), last_executed, ack->seqno());
		exit(1);
	    }
	}
	rms.state = state_acked;

	R_Reply* rep = replies.reply(rms.req->client_id());
	R_Reply_rep* rr = (R_Reply_rep*)(rep?rep->contents():NULL);

	if (rms.state == state_acked && distance(rms.req->replica_id(), id()) == RANSWER) {
#ifdef REPLY_BY_PRIMARY
	    R_Deliver rd(cid, rms.req->seqno(), rms.req->request_id(), rms.req->digest());
#ifdef USE_MACS
	    rms.req->authenticate(id(), rms.authentication_offset, rr, ROUND3);
	    rms.req->copy_authenticators(rd.contents()+sizeof(R_Deliver_rep));
#endif
	    int len = rd.size();
	    send_all(out_socket, rd.contents(), &len);
#else
#ifdef USE_MACS
	    if (!rms.req->is_batched())
	    {
		rms.req->authenticate(id(), rms.authentication_offset, rr, ROUND2);
		rms.req->copy_authenticators(ack->contents()+sizeof(R_ACK_rep));
	    }
#endif
	    R_ACK::Iterator ait = ack->begin();
	    R_Request::Iterator rit = rms.req->begin();
	    while (ait != ack->end()) {
		//fprintf(stderr, "Replica %d: ************* [[[[[[[[[[[[[[[[[[[[ SEND REPLY TO CLIENT, ndx = %d\n", id(), rit.data.index);
#ifdef USE_MACS
		if (rms.req->is_batched()) {
		    R_Reply *rep = replies.reply(rit.data.req->client_id());
		    R_Reply_rep *rr = (R_Reply_rep*)(rep?rep->contents():NULL);
		    if (rit.data.req != rit.data.oreq)
			rit.data.req->replace_authenticators(ait.data.req->contents()+sizeof(R_ACK_rep));
		    int new_offset;
		    rit.data.req->authenticate_for_client(id(), 0, &new_offset, rr, ROUND2);
		    if (rit.data.req == rit.data.oreq)
			rit.data.req->patch_authenticators_for_client(ait.data.req->contents()+sizeof(R_ACK_rep), new_offset);
		    else
			rit.data.req->copy_authenticators(ait.data.req->contents()+sizeof(R_ACK_rep));
		}
#endif
		send_reply_to_client(rit.data.req);
		++ait;
		++rit;
	    }
#endif
	    rms.state = state_stable;
#ifdef REPLY_BY_PRIMARY
	} else if (rms.state == state_acked && rms.req->replica_id() == id()) {
	    rms.state = state_wait_deliver;
#endif
	} else if (rms.state == state_acked) {
	    rms.state = state_stable;
	}

	int replica_id = areq->replica_id();
	int rms_authentication_offset = rms.authentication_offset;
	// now, forward ack, if needed
	if (id() != (replica_id + n() - 1)%n()) {
#ifdef USE_MACS
	    R_Reply* rep = replies.reply(cid);
	    R_Reply_rep* rr = (R_Reply_rep*)(rep?rep->contents():NULL);
	    areq->authenticate(id(), rms_authentication_offset, rr, ROUND2);
	    areq->copy_authenticators(ack->contents()+sizeof(R_ACK_rep));

	    if (rms.req->is_batched())
	    {
		//fprintf(stderr, "R_Replica[%d]: got batched request, ack is batched: %d\n", id(), ack->is_batched());
		R_ACK::Iterator ait = ack->begin();
		R_Request::Iterator rit = rms.req->begin();
		while (ait != ack->end()) {
		    //if (ait.data.req != ait.data.oreq) {
			R_Reply *rep = replies.reply(rit.data.req->client_id());
			R_Reply_rep *rr = (R_Reply_rep*)(rep?rep->contents():NULL);
			int new_offset = 0;
			if (rit.data.req != rit.data.oreq || new_offset < 0)
			{
			    rit.data.req->replace_authenticators(ait.data.req->contents()+sizeof(R_ACK_rep));
			    rit.data.req->authenticate_for_client(id(), 0, &new_offset, rr, ROUND2);
			    rit.data.req->copy_authenticators(ait.data.req->contents()+sizeof(R_ACK_rep));
			} else {
			    int fake_offset = 0;
			    if (distance(id(), rit.data.oreq->replica_id()) == f()+1)
				fake_offset = (R_UMAC_size + R_UNonce_size);
			    rit.data.req->authenticate_for_client(id(), fake_offset, &new_offset, rr, ROUND2);
			    //rit.data.req->patch_authenticators_for_client(ait.data.req->contents()+sizeof(R_ACK_rep), new_offset);
			    //fprintf(stderr, "R_Replica[%d]: doing cafc, offset %d\n", id(), new_offset);
			}
		    //}
		    //fprintf(stderr, "R_Replica[%d]: forwarding req with auths for batched request for client %d, carried by req of client %d\n", id(), rit.data.req->client_id(), rms.req->client_id());
		    ++ait;
		    ++rit;
		}
	    }
#endif

	    int len = ack->size();
	    int err = send_all(out_socket, ack->contents(), &len);
	}
#ifdef REPLY_BY_PRIMARY
	if (id() == areq->replica_id())
	{
	    tosend[cid] = areq;
	    //fprintf(stderr, "R_Replica[%d]: Stored request for %d in tosend\n", id(), cid);
	}
#endif

	if (rms.state == state_stable)
	{
	    if (rms.req)
		delete rms.req;
	    requests.erase(it);
	}

	delete ack;
	return;
}

#ifdef REPLY_BY_PRIMARY
// R_Deliver is an optimization
// it carries the data from the ACK message, and there is no need to verify/authenticate it
// In real deployment, where we have two nics, this won't be used at all,
// so, there is no need to check it.
// client will check as is previous replica sent the message
void R_Replica::handle(R_Deliver *rd)
{
	int cid = rd->client_id();
#ifdef TRACES
	fprintf(stderr, "*********** R_Replica %d: Receiving Deliver(%d) to handle\n", id(), cid);
#endif

	if (tosend.find(cid) != tosend.end()) {
	    // good, it is there
	    R_Request* areq = tosend[cid];
	    tosend[cid] = NULL;
	    tosend.erase(cid);
	    if (areq == NULL) {
		fprintf(stderr, "R_Replica[%d]: problem with areq, it is NULL\n", id());
		delete rd;
		return;
	    }
	    R_Message_Id rmid(cid, areq->request_id());
	    R_Message_Map::iterator it;
	    it = requests.find(rmid);
	    if (it == requests.end()) {
		fprintf(stderr, "R_Replica[%d]: could not find request <%d,%lld> in map\n", id(), cid, areq->request_id());
		exit(1);
	    }

	    R_Message_State& rms = it->second;

#ifdef USE_MACS
	    areq->replace_authenticators(rd->contents()+sizeof(R_Deliver_rep));
#endif
	    R_Reply* rep = replies.reply(areq->client_id());
	    R_Reply_rep* rr = (R_Reply_rep*)(rep?rep->contents():NULL);

	    //if (rms.req->is_batched()) {
		R_Request::Iterator rit = rms.req->begin();
		while (rit != rms.req->end()) {
		    //fprintf(stderr, "************* [[[[[[[[[[[[[[[[[[[[ SEND REPLY TO CLIENT\n");
		    send_reply_to_client(rit.data.req);
		    ++rit;
		}
	    //send_reply_to_client(areq);
	    //fprintf(stderr, "R_Replica[%d]: removing stable request <%d,%lld,%lld>\n", id(), cid, areq->request_id(), areq->seqno());
	    delete areq;
	    rms.state = state_stable;
	    requests.erase(it);
	} else {
	    fprintf(stderr, "R_Replica[%d]: To deliver request is not there!\n", id());
	}
	delete rd;
}
#endif

void R_Replica::handle(R_Checkpoint *c)
{
}

void R_Replica::c_client_requests_handler()
{
	struct timeval timeout;
	int readsocks;
	fd_set socks;

	fprintf(stderr,"Iam here\n");
	listen(in_socket_for_clients, MAX_CONNECTIONS);

	int highsock = in_socket_for_clients;
	connectmap.clear();

	while (1)
	{
		FD_ZERO(&socks);
		FD_SET(in_socket_for_clients, &socks);

		// Loop through all the possible connections and add
		// those sockets to the fd_set
		google::dense_hash_map<int,int>::iterator it;
		for (it = connectmap.begin(); it != connectmap.end(); it++) {
		    FD_SET(it->second, &socks);
		    if (it->second > highsock)
			highsock = it->second;
		}

		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		readsocks = select(highsock + 1, &socks, (fd_set *) 0, (fd_set *) 0,
				&timeout);
		if (readsocks < 0)
		{
			fprintf(stderr, "select returned <0");
			exit(1);
		}
		if (readsocks == 0)
		{
			//fprintf(stderr, ".");
			fflush(stdout);;
		} else
		{
			if (FD_ISSET(in_socket_for_clients, &socks))
			{
				handle_new_connection();
			}

			// Run through the sockets and check to see if anything
			// happened with them, if so 'service' them
			for (it = connectmap.begin(); it != connectmap.end(); it++) {
			    if (FD_ISSET(it->second, &socks))
			    {
				int err;
				R_Message* m = R_Node::recv(it->second, &err);
				if (m == NULL) {
				    //fprintf(stderr, "Miss from %d, %d\n", it->first, it->second);
				    if (err == 0)
					connectmap.erase(it);
				    continue;
				}
				pthread_mutex_lock(&incoming_queue_mutex);
				{
				    //fprintf(stderr, "Got the mutex, appending to queue from %d, on %d\n", it->first, it->second);
				    incoming_queue_clients.append(m);
				    incoming_queue_signaler[1] = 1;
				    pthread_cond_signal(&not_empty_incoming_queue_cond);
				}
				pthread_mutex_unlock(&incoming_queue_mutex);
			    }
			}
		}
	}
	pthread_exit(NULL);
}

void R_Replica::handle_new_connection()
{
	int connection;

	// There is a new connection coming in
	// We  try to find a spot for it in connectlist
	connection = accept(in_socket_for_clients, NULL, NULL);
	if (connection < 0)
	{
		perror("accept");
		exit(EXIT_FAILURE);
	}
	int client_id;
	int res = recvfrom(connection, &client_id, sizeof(int), 0, 0, 0);
	setnonblocking(connection);
	if (connectmap.size() <  MAX_CONNECTIONS) {
	    fprintf(stderr, "\nR_Replica[%d]: Connection accepted:   FD=%d; client_id=%d\n", id(), connection, client_id);
	    connectmap[client_id] = connection;
	    return;
	}

	fprintf(stderr, "\nNo room left for new client.\n");
	close(connection);
}
