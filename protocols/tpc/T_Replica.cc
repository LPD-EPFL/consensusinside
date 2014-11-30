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
#include <map>

#include "th_assert.h"
#include "T_Replica.h"
#include "T_Message_tags.h"
#include "T_Reply.h"
#include "T_Principal.h"
#include "T_Request.h"
#include "MD5.h"

#include "T_Certificate.t"
template class T_Certificate<T_Ack>;
template class T_Certificate<T_CommitAck>;
// Global replica object.
T_Replica *T_replica;

#define printDebug(...) \
    do { \
	struct timeval tv; \
	gettimeofday(&tv, NULL); \
	fprintf(stderr, "%u.%06u: ", tv.tv_sec, tv.tv_usec); \
	fprintf(stderr, __VA_ARGS__); \
    } while (0);
#undef printDebug
#define printDebug(...) \
    do { } while (0);

T_Replica::T_Replica(FILE *config_file, FILE *config_priv, char *host_name, short req_port) :
	T_Node(config_file, config_priv, host_name, req_port), seqno(0), r_acks(num_replicas-1), r_cacks(num_replicas-1) 
{
	// Fail if node is not a replica.
	if (!is_replica(id()))
	{
		th_fail("Node is not a replica");
	}

	T_replica = this;

	replies = new T_Rep_info(num_principals);

	//Added by Maysam Yabandeh
	servicingIndex = -1;
   outstandingMsg = new T_Request*[num_principals];
	for (int i = 0; i < num_principals; i++)
		outstandingMsg[i] = NULL;

	// Read view change, status, and recovery timeouts from replica's portion
	// of "config_file"
	int vt, st, rt;
	fscanf(config_file, "%d\n", &vt);
	fscanf(config_file, "%d\n", &st);
	fscanf(config_file, "%d\n", &rt);

	// Create timers and randomize times to avoid collisions.
	srand48(getpid());

	//join_mcast_group();

	// Disable loopback
	//u_char l = 0;
	//int error = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &l, sizeof(l));
	//if (error < 0)
	//{
		//perror("unable to disable loopback");
		//exit(1);
	//}

   serverListen();
	fprintf(stderr, "Created the task for receiving client requests\n");
}

T_Replica::~T_Replica()
{
	delete replies;
	delete outstandingMsg;
	//delete rh;
}

void T_Replica::register_exec(int(*e)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool))
{
	exec_command = e;
}

//#define recv_dbg
//#define maysamdbg
//#define TRACES

void T_Replica::handle(T_Request *m, int sock) {
#ifdef maysamdbg
	fprintf(stderr, "===Replica: T_Request: id=%d primary=%d rid=%d\n", id(), primary(), m->request_id());
	fprintf(stderr, "1");
#endif
   principals[m->sender_id()]->remotefd = sock;
	th_assert(outstandingMsg[m->client_id()] == NULL, "request for another outstanding request");
	th_assert(servicingIndex == -1 || primary() == id(), "receiving request in a non-primary node while servicing another one");
	outstandingMsg[m->client_id()] = m;
	if (servicingIndex != -1)
	{
		outstandingMsgIndex.push_back(m->client_id());
		return;
	}
	servicingIndex = m->client_id();
	processRequest(m);
}

void T_Replica::processRequest(T_Request *m) {
   r_acks.clear();
	r_cacks.clear();

	if (m->is_read_only())
	{
	    fprintf(stderr, "T_Replica::handle(): read-only requests are not handled.\n");
	    delete m;
	    return;
	}

	int cid = m->client_id();
	Request_id rid = m->request_id();

#ifdef maysamdbg
	fprintf(stderr, "7");
#endif
#ifdef TRACES
	fprintf(stderr, "T_Replica::handle() (cid = %d, rid=%llu or %d)\n", cid, rid, rid);
#endif


   if (id() == primary())
	{ //forward the requrst to all nodes
	   m->rep().sender_id = id();
		T_node->send(m, T_All_replicas);
		return;
	}
	//else  not primary

	Request_id last_rid = replies->req_id(cid);
	if (last_rid + 1 != rid)
	{
			fprintf(stderr, "T_Replica.cc: Expecting %llu receiving %llu\n", last_rid + 1, rid);
			exit(1);
	}
#ifdef maysamdbg
	fprintf(stderr, "8");
#endif
   T_Ack ack = T_Ack(rid);
	T_node->send(&ack, primary());
#ifdef maysamdbg
	fprintf(stderr, "11");
#endif
}

//recv ack
void T_Replica::handle(T_Ack *m, int sock) {
#ifdef maysamdbg
	fprintf(stderr, "===Replica: T_Ack: id=%d primary=%d \n", id(), primary());
#endif
	th_assert(servicingIndex != -1, "ack for a non-received request");
	th_assert(id() == primary(), "ack to a non-primary node");
	//TODO also check for client ids, rid are not unique
	assert(m->rep().rid == outstandingMsg[servicingIndex]->rep().rid);
	r_acks.add(m, T_node);

   if (r_acks.is_complete())
	{
		T_Commit commit = T_Commit(m->rep().rid);
		T_node->send(&commit, T_All_replicas);
		if (!execute_request(outstandingMsg[servicingIndex]))
			fprintf(stderr, "T_Replica.cc: Error in executing the request 2\n");
	}
	return;
}

//recv commit
void T_Replica::handle(T_Commit *m, int sock) {
#ifdef maysamdbg
	fprintf(stderr, "===Replica: T_Commit: id=%d primary=%d \n", id(), primary());
#endif
	th_assert(servicingIndex != -1, "commit for a non-received request");
	th_assert(outstandingMsg[servicingIndex] != NULL, "commit for a non-received request");
	th_assert(id() != primary(), "commit to a primary node");

	if (!execute_request(outstandingMsg[servicingIndex]))
		fprintf(stderr, "T_Replica.cc: Error in executing the request\n");
   delete m;
	delete outstandingMsg[servicingIndex];
	outstandingMsg[servicingIndex] = NULL;
	servicingIndex = -1;
   T_CommitAck cack = T_CommitAck(m->rep().rid);
	T_node->send(&cack, primary());
	return;
}

//recv commit ack
void T_Replica::handle(T_CommitAck *m, int sock) {
#ifdef maysamdbg
	fprintf(stderr, "===Replica: T_CommitAck: id=%d primary=%d \n", id(), primary());
#endif
	th_assert(servicingIndex != -1, "commit for a non-received request");
	th_assert(outstandingMsg[servicingIndex] != NULL, "commit for a non-received request");
	th_assert(id() == primary(), "commit to a primary node");
	assert(m->rep().rid == outstandingMsg[servicingIndex]->rep().rid);
	r_cacks.add(m, T_node);

   if (r_cacks.is_complete())
	{
		// Send full reply.
		int cid = outstandingMsg[servicingIndex]->client_id();
		delete outstandingMsg[servicingIndex];
		outstandingMsg[servicingIndex] = NULL;
		servicingIndex = -1;
#ifdef TRACES
		fprintf(stderr, "Replica::execute_prepared: %d Sending full reply \n", id());
#endif
		replies->send_reply(cid, 0, id(), true);
		if (outstandingMsgIndex.size() > 0)
		{
		   servicingIndex = *outstandingMsgIndex.begin();
			outstandingMsgIndex.erase(outstandingMsgIndex.begin());
			assert(outstandingMsg[servicingIndex] != NULL);
			processRequest(outstandingMsg[servicingIndex]);
		}
	}
	return;
}

bool T_Replica::execute_request(T_Request *req)
{
#ifdef maysamdbg
	fprintf(stderr, "11 ");
#endif
	int cid = req->client_id();
	Request_id rid = req->request_id();

	if (replies->req_id(cid) + 1 != rid)
	{
		fprintf(stderr, "execute_request error: id=%d cid=%d rid=%llu expected rid=%llu\n", id(), cid, rid, replies->req_id(cid) + 1);
		return false;
	}

#ifdef maysamdbg
	fprintf(stderr, "12 ");
#endif
	// Obtain "in" and "out" buffers to call exec_command
	Byz_req inb;
	Byz_rep outb;
	Byz_buffer non_det;
	inb.contents = req->command(inb.size);
	outb.contents = replies->new_reply(cid, outb.size);
#ifdef maysamdbg
	fprintf(stderr, "1size=%d \n", outb.size);
#endif

	// Execute command in a regular request.
	exec_command(&inb, &outb, &non_det, cid, false);
#ifdef maysamdbg
	fprintf(stderr, "3size=%d \n", outb.size);
#endif

	// Finish constructing the reply.
	replies->end_reply(cid, rid, outb.size);

	return true;
}

void T_Replica::join_mcast_group()
{
exit(1);
	//struct ip_mreq req;
	//req.imr_multiaddr.s_addr = group->address()->sin_addr.s_addr;
	//req.imr_interface.s_addr = INADDR_ANY;
	//int error = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &req,
			//sizeof(req));
	//if (error < 0)
	//{
		//perror("Unable to join group");
		//exit(1);
	//}
}

void T_Replica::leave_mcast_group()
{
exit(1);
	//struct ip_mreq req;
	//req.imr_multiaddr.s_addr = group->address()->sin_addr.s_addr;
	//req.imr_interface.s_addr = INADDR_ANY;
	//int error = setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *) &req,
			//sizeof(req));
	//if (error < 0)
	//{
		//perror("Unable to leave group");
		//exit(1);
	//}
}

void T_Replica::processReceivedMessage(T_Message* msg, int sock) 
{
#ifdef maysamdbg
fprintf(stderr, "=T_Replica::processReceivedMessage\n");
#endif
		switch (msg->tag())
		{
		    case T_Request_tag:
			gen_handle<T_Request>(msg, sock);
			break;
		    case T_Ack_tag:
			gen_handle<T_Ack>(msg, sock);
			break;
		    case T_CommitAck_tag:
			gen_handle<T_CommitAck>(msg, sock);
			break;
		    case T_Commit_tag:
			gen_handle<T_Commit>(msg, sock);
			break;

		    default:
fprintf(stderr, "=T_Replica::processReceivedMessage Error: Unknown msg tag\n");
exit(1);
			// Unknown message type.
			delete msg;
		}
}
