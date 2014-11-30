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
#include "O_Replica.h"
#include "O_Message_tags.h"
#include "O_Reply.h"
#include "O_Principal.h"
#include "MD5.h"

#include "O_M_Replica.h"
#include "task.h"

#include "O_Certificate.t"
// Global replica object.
O_Replica *O_replica;

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

seq_t getNextSeq();
O_Replica::O_Replica(FILE *config_file, FILE *config_priv, char *host_name, short req_port) :
	O_Node(config_file, config_priv, host_name, req_port), seqno(0)  
{
	// Fail if node is not a replica.
	if (!is_replica(id()))
	{
		th_fail("Node is not a replica");
	}

	O_replica = this;

	replies = new O_Rep_info(num_principals);

	// Read view change, status, and recovery timeouts from replica's portion
	// of "config_file"
	int vt, st, rt;
	fscanf(config_file, "%d\n", &vt);
	fscanf(config_file, "%d\n", &st);
	fscanf(config_file, "%d\n", &rt);

	// Create timers and randomize times to avoid collisions.
	srand48(getpid());


  IamLeader=false;//If I think that I am the leader
  activeAcceptor = -1; // the address of the active acceptor which I am using
  highest_proposal_number = 0;//the smallest possible value for proposal number
  next_uncommited_instance_number_var = 0;
  lastTimeIgaveupLeadership = 0;
  waitingForProposalChange = false;
 
  //init
  //TODO: remove it
  if (id() == 0)
  {
	  IamLeader = true;
	  activeAcceptor = 1;
  }


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
	taskdelay(1000);//make sure the others have created their listen queue
	for (int i = id(); i >= 0; i--)
		O_node->justDial(i);
	//this is to i) avoid making double i-j and j-i connection at the same time ii) make sure we have 2->0 connection
	fprintf(stderr, "Created the task for receiving client requests\n");
}

O_Replica::~O_Replica()
{
	delete replies;
}

void O_Replica::register_exec(int(*e)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool))
{
	exec_command = e;
}

//#define recv_dbg
//#define maysamdbg
//#define TRACES

void O_Replica::propose(uint64_t index, O_Request *m) {
	//Added by Maysam Yabandeh - kind of mutex
   while (waitingForProposalChange)
		taskyield();
	waitingForProposalChange = true;
	if(IamLeader){
		O_Accept *acc = (O_Accept*) m;
		acc->convert();//set the tag
		acc->index() = index;
		acc->n() = globaln;
		O_node->send(acc, activeAcceptor);
		waitingForProposalChange = false;
	}
	else {
		if (activeAcceptor == -1)
		{
		   //th_assert(0, "it is not implemented yet");
			uint64_t instance;
			AcceptedProposals lastLeaderUncommiteProposals;
			int acceptor;
			O_M_replica->lastActiveAcceptor(acceptor, instance, lastLeaderUncommiteProposals);
			fprintf(stderr, "try to change the leader acceptor=%d index=%lld\n", acceptor, instance);
			bool success = O_M_replica->proposeLeaderChange(instance);
			assert(success);
			fprintf(stderr, "... I (%d) am leader now for client %d\n", id(), m->sender_id());
			activeAcceptor = acceptor;
			//TODO: In the case of failure, I can do something
			registerProposals(lastLeaderUncommiteProposals);
		}
		globaln = getNextSeq();
		O_Prepare prep(index, globaln);
		O_node->send(&prep, activeAcceptor);
	}
} // propose

void O_Replica::handle(O_Request *m, int sock) {
#ifdef maysamdbg
	fprintf(stderr, "===Replica: O_Request: id=%d primary=%d rid=%lld\n", id(), primary(), m->request_id());
#endif
   principals[m->sender_id()]->remotefd = sock;
	processRequest(m);
}

void O_Replica::processRequest(O_Request *m) {
   if (!IamLeader)
		if (time(NULL) - lastTimeIgaveupLeadership < 5)//5 seconds
		{
			uint64_t instance;
			int leader;
			O_M_replica->lastLeader(leader, instance);
			fprintf(stderr, "=== === == id(%d) sending O_Abandon to %d\n", id(), m->sender_id());
			O_Abandon aba(highest_proposal_number, leader);
			O_node->send(&aba, m->sender_id());
			delete m;
			return;
		}
	if (m->is_read_only())
	{
	    fprintf(stderr, "O_Replica::handle(): read-only requests are not handled.\n");
	    delete m;
	    return;
	}

	//int cid = m->client_id();
	//TODO: see if the request is repetetive
	//Request_id rid = m->request_id();

#ifdef TRACES
	int cid = m->client_id();
	Request_id rid = m->request_id();
	fprintf(stderr, "O_Replica::handle() (cid = %d, rid=%llu)\n", cid, rid);
#endif

	//forward the requrst to all nodes
	m->sender_id() = id();
	uint64_t index;
	//first sending pending proposals
	index = next_uncommited_instance_number();
	std::map<uint64_t, O_Request*>::iterator i = proposals.find(index);
	th_assert(i == proposals.end(), "try to set already used index");
	std::map<uint64_t, O_Learn*>::iterator ci = chosenValues.find(index);
	th_assert(ci == chosenValues.end(), "try to set already learned index");
	//then its turn for this request
	//
	//
	//bug
	//if (id() == 0 && index == 50)
		//while (1) taskdelay(100000);
	//
	proposals[index] = m;
	propose(index, m);
	return;
}

void O_Replica::handle(O_Prepare *m, int sock) {
#ifdef maysamdbg
	fprintf(stderr, "===Replica: O_Prepare: id=%d sender=%d\n", id(), m->sender_id());
#endif
   principals[m->sender_id()]->remotefd = sock;

	//std::map<uint64_t, O_Accept*>::const_iterator i = accepted.find(m->index());
	//if (i == accepted.end()) {
		if (highest_proposal_number < m->n())
		{
			fprintf(stderr, "id(%d) %d < %d\n", id(), highest_proposal_number, m->n());
			highest_proposal_number = m->n();
			AcceptedProposals ap(accepted, chosenValues);
			O_PrepareResponse prepResp(m->index(), m->n(), ap);
			O_node->send(&prepResp, m->sender_id());
		}
		else
		{
			uint64_t instance;
			int leader;
		   O_M_replica->lastLeader(leader, instance);
			//fprintf(stderr, "=========sening O_Abandon %d < %d\n", highest_proposal_number, m->n());
			O_Abandon aba(highest_proposal_number, leader);
			O_node->send(&aba, m->sender_id());
		}
	//}
	//else
	//{
		//O_Learn *learn = (O_Learn*) i->second;
		//learn->convert();
		//O_node->send(learn, O_All_replicas);
	//}
} // deliver Prepare

void O_Replica::handle(O_PrepareResponse *m, int sock) {
#ifdef maysamdbg
	fprintf(stderr, "===Replica: O_PrepareResponse: id=%d sender=%d\n", id(), m->sender_id());
#endif
   principals[m->sender_id()]->remotefd = sock;
	//Added by Maysam Yabandeh
	//perhaps late messages
	if (IamLeader) return;
	if (activeAcceptor != m->sender_id()) return;
	waitingForProposalChange = false;//TODO clean this crap

	//When I become leader, I have to start proposing
	IamLeader = true;
	registerProposals(m->acceptedProposals());
	map<uint64_t, O_Request*>::iterator i = proposals.find(m->index());
	assert (i != proposals.end());
	O_Accept *acc = (O_Accept*) i->second;
	acc->convert();//set the tag
	acc->index() = m->index();
	acc->n() = m->n();
	O_node->send(acc, activeAcceptor);
} // deliver PrepareResponse

void O_Replica::handle(O_Accept *m, int sock) {
#ifdef maysamdbg
	fprintf(stderr, "===Replica: O_Accept: id=%d sender=%d rid=%llu\n", id(), m->sender_id(), m->request_id());
#endif
bool hack = m->sender_id() == 2;
		//if (hack)
	//fprintf(stderr, "===Replica: O_Accept: id=%d sender=%d rid=%llu\n", id(), m->sender_id(), m->request_id());
	if (m->n() >= highest_proposal_number) {
			//fprintf(stderr, "id(%d) %d <= %d\n", id(), highest_proposal_number, m->n());
		accepted[m->index()] = m;
		O_Learn *l = (O_Learn*) m;
		l->convert();//set the tag
		//hack for addressing the problem of full queue of 0
		if (!hack)
			O_node->send(l, O_All_replicas);
		else
		{
			O_node->send(l, 1);
			O_node->send(l, 2);
			//to simulate the cost of sending to 0
			O_node->send(l, 2);
			O_node->send(l, 2);
		}
	}
	else {
		uint64_t instance;
		int leader;
		O_M_replica->lastLeader(leader, instance);
			//fprintf(stderr, "===qq === == id(%d) sending O_Abandon to %d\n", id(), m->sender_id());
			//fprintf(stderr, "id(%d) %d <= %d\n", id(), highest_proposal_number, m->n());
		O_Abandon aba(highest_proposal_number, leader);
		O_node->send(&aba, m->sender_id());
	}
} // deliver O_Accept

void O_Replica::handle(O_Learn *m, int sock) {
#ifdef maysamdbg
	fprintf(stderr, "===Replica: O_Learn: id=%d index=%llu rid=%llu sender=%d\n", id(), m->index(), m->request_id(), m->sender_id());
#endif
   principals[m->sender_id()]->remotefd = sock;

	map<uint64_t, O_Learn*>::const_iterator ci = chosenValues.find(m->index());
	if (ci != chosenValues.end()) {
		return;
	}
	chosenValues[m->index()] = m;
	update_next_uncommited_instance_number(m->index());
	int cid = m->client_id();
#ifdef TRACES
	fprintf(stderr, "Replica::execute_prepared: %d Sending full reply \n", id());
#endif
   //if (!IamLeader && activeAcceptor != id())//there is less load on this node
   if (IamLeader)//there is already an open connection. client do not listen to new connections
	{
		execute_request(m);
		replies->send_reply(cid, 0, id(), true);
	}
	else
		execute_request(m);
} // deliver O_Learn

void O_Replica::handle(O_Abandon *m, int sock) {
#ifdef maysamdbg
	fprintf(stderr, "===Replica: O_Abandon: id=%d sender=%d\n", id(), m->sender_id());
#endif
	//I am not leader apparently, so lets tell back to the client
	if (m->leader_id() == id()) 
	{
		fprintf(stderr, "I am still leader! ignore the O_Abandon\n");
		return; //I am fine, then something else is wrong
	}
	//TODO: keep trying sending the request
	if (!IamLeader) 
	{
		fprintf(stderr, "I am not already leader! ignore the O_Abandon\n");
		return;//probabely I have already notified the clients
	}
	IamLeader = false;
	lastTimeIgaveupLeadership = time(NULL);
	m->sender_id() = id();
	std::map<uint64_t, O_Request*>::iterator i = proposals.begin();
	//TODO make it more efficient
	for (; i != proposals.end(); i++)
	{
		int64_t index = i->first;
		int& cid = i->second->client_id();
		std::map<uint64_t, O_Learn*>::iterator ci = chosenValues.find(index);
		if (ci == chosenValues.end()) //proposed but not chosen
		{
#ifdef maysamdbg
		   fprintf(stderr, "----------- I (%d) notify client %d of leader change to %d\n", id(), cid, m->leader_id());
#endif
			O_node->send(m, cid);
		}
#ifdef maysamdbg
		else
		   fprintf(stderr, "----------- proposal %lld is already chosen for client  %d\n", index, cid);
#endif
	}
}

seq_t O_Replica::getNextSeq() {
//TODO fix this
   static int counter = 0;
	counter ++;
	counter = counter << 2 | id();
	struct timeval t;
	gettimeofday(&t, 0);
	int r = (t.tv_sec << 16) | counter;
	r = r & ~(1 << 31);
#ifdef maysamdbg
	fprintf(stderr, "getNextSeq() time=%ld r=%d id=%d\n", t.tv_sec, r, id());
#endif
	return r;
}

void O_Replica::update_next_uncommited_instance_number(uint64_t commitedInstance) {
	uint64_t next = commitedInstance + 1;
	if (next > next_uncommited_instance_number_var)
		next_uncommited_instance_number_var = next;
	//TODO: what if there is a gap between them
}

uint64_t O_Replica::next_uncommited_instance_number() {
	return next_uncommited_instance_number_var++;
	//TODO: What if I am using UDP and some requests fail
}

void O_Replica::registerProposals(AcceptedProposals& ap)
{
   for (int i = 0; i < ap.size; i++)
		acceptedProposals.push_back(ap.list[i]);
}

bool O_Replica::execute_request(O_Learn *req)
{
#ifdef maysamdbg
	fprintf(stderr, "11 ");
#endif
	int cid = req->client_id();
	Request_id rid = req->request_id();

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

void O_Replica::join_mcast_group()
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

void O_Replica::leave_mcast_group()
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

void O_Replica::processReceivedMessage(O_Message* msg, int sock) 
{
#ifdef maysamdbg
fprintf(stderr, "=O_Replica::processReceivedMessage\n");
#endif
		switch (msg->tag())
		{
		    case O_Request_tag:
			gen_handle<O_Request>(msg, sock);
			break;
		    case O_Accept_tag:
			gen_handle<O_Accept>(msg, sock);
			break;
		    case O_Learn_tag:
			gen_handle<O_Learn>(msg, sock);
			break;
		    case O_Prepare_tag:
			gen_handle<O_Prepare>(msg, sock);
			break;
		    case O_PrepareResponse_tag:
			gen_handle<O_PrepareResponse>(msg, sock);
			break;
		    case O_Abandon_tag:
			gen_handle<O_Abandon>(msg, sock);
			break;

		    default:
fprintf(stderr, "=O_Replica::processReceivedMessage Error: Unknown msg tag\n");
exit(1);
			// Unknown message type.
			delete msg;
		}
}
