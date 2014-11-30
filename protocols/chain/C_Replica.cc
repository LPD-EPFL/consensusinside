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

#include "th_assert.h"
#include "C_Message_tags.h"
#include "C_Request.h"
#include "C_Checkpoint.h"
#include "C_Reply.h"
#include "C_Principal.h"
#include "C_Replica.h"
#include "MD5.h"

#ifdef REPLY_BY_PRIMARY
#include "C_Deliver.h"
#endif

//#define TRACES

// Global replica object.
C_Replica *C_replica;

// Function for handling enqueued requests
//extern "C"
void*request_queue_handler_helper(void *o)
{
	void **o2 = (void **)o;
	C_Replica &r = (C_Replica&)(*o2);
	r.request_queue_handler();
	return 0;
}

// Function for the thread receiving messages from the predecessor in the chain
//extern "C"
void*requests_from_predecessor_handler_helper(void *o)
{
	void **o2 = (void **)o;
	C_Replica &r = (C_Replica&)(*o2);
	r.requests_from_predecessor_handler();
	return 0;
}

// Function for the thread receiving messages from clients
//extern "C"
void*C_client_requests_handler_helper(void *o)
{
	void **o2 = (void **)o;
	C_Replica &r = (C_Replica&)(*o2);
	r.c_client_requests_handler();
	return 0;
}

inline void*C_message_queue_handler_helper(void *o)
{
	void **o2 = (void **)o;
	C_Replica &r = (C_Replica&) (*o2);
	//temp_replica_class = (Replica<class Request_T, class Reply_T>&)(*o);
	//  r.recv1();
	//temp_replica_class.do_recv_from_queue();
	r.do_recv_from_queue();
	return 0;
}

C_Replica::C_Replica(FILE *config_file, FILE *config_priv, char* host_name, short port) :
	C_Node(config_file, config_priv, host_name, port), seqno(0), replies(num_principals), checkpoint_store()
{
	// Fail if node is not a replica.
	if (!is_replica(id()))
	{
		th_fail("Node is not a replica");
	}

	seqno = 0;

	// Read view change, status, and recovery timeouts from replica's portion
	// of "config_file"
	int vt, st, rt;
	fscanf(config_file, "%d\n", &vt);
	fscanf(config_file, "%d\n", &st);
	fscanf(config_file, "%d\n", &rt);

	// Create timers and randomize times to avoid collisions.
	srand48(getpid());

	exec_command = 0;

	// Create server socket
	in_socket= createServerSocket(ntohs(principals[node_id]->TCP_addr.sin_port));

	// Create receiving thread
	if (pthread_create(&requests_from_predecessor_handler_thread, 0,
			&requests_from_predecessor_handler_helper, (void *)this) != 0)
	{
		fprintf(stderr, "Failed to create the thread for receiving messages from predecessor in the chain\n");
		exit(1);
	}

	// Connect to principals[(node_id + 1) % num_r]
	out_socket = createClientSocket(principals[(node_id + 1) % num_replicas]->TCP_addr);

	if (node_id == 0 || node_id == (num_replicas - 1))
	{
		fprintf(stderr,"Creating client socket\n");
		in_socket_for_clients= createNonBlockingServerSocket(ntohs(principals[node_id]->TCP_addr_for_clients.sin_port));
		if (pthread_create(&C_client_requests_handler_thread, NULL,
				&C_client_requests_handler_helper, (void *)this)!= 0)
		{
			fprintf(stderr, "Failed to create the thread for receiving client requests\n");
			exit(1);
		}
	}
}

C_Replica::~C_Replica()
{
}

void C_Replica::do_recv_from_queue()
{
	C_Message *m;
	while (1)
	{
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
		handle(m);
	}
}

void C_Replica::register_exec(int(*e)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool))
{
	exec_command = e;
}

void C_Replica::requests_from_predecessor_handler()
{
	int socket = -1;

	listen(in_socket, 1);
	socket = accept(in_socket, NULL, NULL);
	if (socket < 0)
	{
		perror("Cannot accept connection \n");
		exit(-1);
	}

	// Loop to receive messages.
	while (1)
	{

		C_Message* m = C_Node::recv(socket);
#ifdef TRACES
		fprintf(stderr, "Received message\n");
#endif
/* absolutely unnecessary
		C_Request *n;
		C_Checkpoint *ch;
		if (m->tag() == C_Request_tag && C_Request::convert(m, n))
		{
#ifdef TRACES
			fprintf(stderr, "Converted message to request\n");
#endif
*/
			// Enqueue the message
			pthread_mutex_lock(&incoming_queue_mutex);
			{
				incoming_queue.append(m);
				pthread_cond_signal(&not_empty_incoming_queue_cond);
			}
			pthread_mutex_unlock(&incoming_queue_mutex);
/* ditto
		} else if (m->tag() == C_Checkpoint_tag && C_Checkpoint::convert(m, ch))
		{
#ifdef TRACES
			fprintf(stderr, "Converted message to checkpoint\n");
#endif
		} else
		{
			delete m;
		}
*/
	}
}

void C_Replica::handle(C_Message *m)
{
	// TODO: This should probably be a jump table.
	switch (m->tag())
	{
		case C_Request_tag:
			gen_handle<C_Request>(m);
			break;

		case C_Checkpoint_tag:
			gen_handle<C_Checkpoint>(m);
			break;

#ifdef REPLY_BY_PRIMARY
		case C_Deliver_tag:
			gen_handle<C_Deliver>(m);
			break;
#endif

default:
			// Unknown message type.
			delete m;
	}
}

void C_Replica::handle(C_Request *req)
{
	bool ro = req->is_read_only();
	int cid = req->client_id();

#ifdef TRACES
	fprintf(stderr, "*********** C_Replica %d: Receiving request to handle\n", id());
#endif

	if (ro)
	{
		fprintf(stderr, "C_Replica %d: Read-only requests are currently not handled\n", id());
		delete req;
		exit(1);
	}

	int authentication_offset;
	if (!req->verify(&authentication_offset))
	{
		fprintf(stderr, "C_Replica %d: verify returned FALSE\n", id());
		delete req;
		exit(1);
	}
#ifdef TRACES
	fprintf(stderr, "*********** C_Replica %d: message verified\n", id());
#endif
	// Execute the request
	Byz_req inb;
	Byz_rep outb;
	inb.contents = req->command(inb.size);
	outb.contents = replies.new_reply(cid, outb.size);

	// Execute command in a regular request.
	exec_command(&inb, &outb, NULL, cid, false);

#ifdef TRACES
	fprintf(stderr, "*********** C_Replica %d: commmand executed\n", id());
#endif
	if (outb.size % ALIGNMENT_BYTES)
	{
		for (int i=0; i < ALIGNMENT_BYTES - (outb.size % ALIGNMENT_BYTES); i++)
		{
			outb.contents[outb.size+i] = 0;
		}
	}

	/*
	Digest d;
	if (rh.add_request(req, seqno, d))
	    seqno++;
	*/

	// Finish constructing the reply.
	C_Reply_rep *rr = replies.end_reply(cid, req->request_id(), outb.size);

	req->authenticate(C_node->id(), authentication_offset, rr);

	if (node_id < num_replicas - 1)
	{
		// Forwarding the request
#ifdef TRACES
		fprintf(stderr, "C_Replica %d: Forwarding the request\n", id());
#endif
		int len = req->size();
		send_all(out_socket, req->contents(), &len);
		//return;
	}
#ifdef REPLY_BY_PRIMARY
	else {
	    // sending deliver to primary
#ifdef TRACES
		fprintf(stderr, "C_Deliver to primary\n");
#endif
	    C_Deliver rd(cid, req->request_id(), req->digest());
	    req->copy_authenticators(rd.contents()+sizeof(C_Deliver_rep));
	    int len = rd.size();
	    send_all(out_socket, rd.contents(), &len);
	}
#else
	else
	{
		// C_Replying to the client
#ifdef TRACES
		fprintf(stderr, "C_Replying to the client\n");
#endif
		replies.send_reply(cid, connectlist[req->listnum()], req->MACs());
		//return;
	}
#endif
#ifdef REPLY_BY_PRIMARY
	if (id() == 0)
	{
	    // store request aside
	    tosend[cid] = req;
	} else
#endif
	    delete req;
	return;

	// send the checkpoint if necessary
	if (rh.should_checkpoint()) {
	    C_Checkpoint chkp;
	    // fill in the checkpoint message
	    chkp.set_seqno(rh.get_top_seqno());
	    chkp.set_digest(rh.rh_digest());
	    // sign
	    C_node->gen_signature(chkp.contents(), sizeof(C_Checkpoint_rep),
		    chkp.contents()+sizeof(C_Checkpoint_rep));
	    // send it
	    //send(chkp, C_All_replicas);
	}

}

void C_Replica::handle(C_Checkpoint *c)
{
    // verify signature
    if (!c->verify()) {
	fprintf(stderr, "Couldn't verify the signature of C_Checkpoint\n");
	delete c;
	return;
    }

    // optimization: if less than last removed, discard
    if (checkpoint_store.last() != 0 && c->get_seqno() < checkpoint_store.last())
    {
	fprintf(stderr, "Checkpoint is for older than last removed, discarding\n");
	delete c;
	return;
    }

    // store
    CheckpointSet *checkpoints;
    if (!checkpoint_store.find(c->get_seqno(), checkpoints)) {
	checkpoints = new CheckpointSet(n());
	checkpoints->store(c);
	checkpoint_store.add(c->get_seqno(), checkpoints);
    } else {
	checkpoints->store(c);
    }
    // check whether full
    // if so, clear, and truncate history
    if (checkpoints->size() == n()) {
	fprintf(stderr, "Collected enough checkpoints\n");
	bool same = false;
	for (int i=0; i<n(); i++) {
	    C_Checkpoint *cc = checkpoints->fetch(i);
	    same = c->match(cc);
	    if (!same)
		break;
	}
	// we should now truncate the history
	if (same) {
	    fprintf(stderr, "All checkpoints are the same\n");
	    checkpoint_store.remove(c->get_seqno());
	    rh.truncate_history(c->get_seqno());
	}
    }
}

#ifdef REPLY_BY_PRIMARY
// C_Deliver is an optimization
// it carries the data about reply to the client, as sent by the last replica
// however, primary will respond
// In real deployment, where we have two nics, this won't be used at all,
// so, there is no need to check it.
// client will check as is previous replica sent the message
void C_Replica::handle(C_Deliver *rd)
{
	int cid = rd->client_id();
#ifdef TRACES
	fprintf(stderr, "*********** C_Replica %d: Receiving Deliver(%d) to handle\n", id(), cid);
#endif

	if (tosend.find(cid) != tosend.end()) {
	    // good, it is there
	    C_Request* areq = tosend[cid];
	    tosend[cid] = NULL;
	    tosend.erase(cid);
	    if (areq == NULL) {
		fprintf(stderr, "C_Replica[%d]: problem with areq, it is NULL\n", id());
		delete rd;
		return;
	    }

	    areq->replace_authenticators(rd->contents()+sizeof(C_Deliver_rep));

	    C_Reply* rep = replies.reply(areq->client_id());
	    C_Reply_rep* rr = (C_Reply_rep*)(rep?rep->contents():NULL);

	    //fprintf(stderr, "************* [[[[[[[[[[[[[[[[[[[[ SEND REPLY TO CLIENT\n");
	    replies.send_reply(cid, connectlist[areq->listnum()], areq->MACs());
	    delete areq;
	} else {
	    fprintf(stderr, "C_Replica[%d]: To deliver request is not there!\n", id());
	}
	delete rd;
}
#endif

void C_Replica::request_queue_handler()
{
	C_Request* m;
	while (1)
	{
		pthread_mutex_lock(&incoming_queue_mutex);
		{
#ifdef TRACES
			fprintf(stderr, "*********** C_Replica %d: waiting for a request\n", id());
#endif
			while (incoming_queue.size() == 0)
			{
				pthread_cond_wait(&not_empty_incoming_queue_cond,
						&incoming_queue_mutex);
			}
			m = (C_Request *) incoming_queue.remove();
		}
		pthread_mutex_unlock(&incoming_queue_mutex);

		bool ro = m->is_read_only();
		int cid = m->client_id();

#ifdef TRACES
		fprintf(stderr, "*********** C_Replica %d: Receiving request to handle\n", id());
#endif

		if (ro)
		{
			fprintf(stderr, "C_Replica %d: Read-only requests are currently not handled\n", id());
			delete m;
			exit(1);
		}

		int authentication_offset;
		if (!m->verify(&authentication_offset))
		{
			fprintf(stderr, "C_Replica %d: verify returned FALSE\n", id());
			delete m;
			exit(1);
		}
#ifdef TRACES
		fprintf(stderr, "*********** C_Replica %d: message verified\n", id());
#endif
		// Execute the request
		Byz_req inb;
		Byz_rep outb;
		inb.contents = m->command(inb.size);
		outb.contents = replies.new_reply(cid, outb.size);

		// Execute command in a regular request.
		exec_command(&inb, &outb, NULL, cid, false);
#ifdef TRACES
		fprintf(stderr, "*********** C_Replica %d: commmand executed\n", id());
#endif
		if (outb.size % ALIGNMENT_BYTES)
		{
			for (int i=0; i < ALIGNMENT_BYTES - (outb.size % ALIGNMENT_BYTES); i++)
			{
				outb.contents[outb.size+i] = 0;
			}
		}

		// Finish constructing the reply.
		C_Reply_rep *rr = replies.end_reply(cid, m->request_id(), outb.size);

		m->authenticate(C_node->id(), authentication_offset, rr);

		if (node_id < num_replicas - 1)
		{
			// Forwarding the request
#ifdef TRACES
			fprintf(stderr, "C_Replica %d: Forwarding the request\n", id());
#endif
			int len = m->size();
			send_all(out_socket, m->contents(), &len);
		} else
		{
			// C_Replying to the client
#ifdef TRACES
			fprintf(stderr, "C_Replying to the client\n");
#endif
			replies.send_reply(cid, connectlist[m->listnum()], m->MACs());
		}
		delete m;
	}
}

void C_Replica::c_client_requests_handler()
{
	struct timeval timeout;
	int readsocks;
	fd_set socks;

	fprintf(stderr,"Iam here\n");
	listen(in_socket_for_clients, MAX_CONNECTIONS);

	int highsock = in_socket_for_clients;
	memset((char *) &connectlist, 0, sizeof(connectlist));

	while (1)
	{
		FD_ZERO(&socks);
		FD_SET(in_socket_for_clients, &socks);

		// Loop through all the possible connections and add
		// those sockets to the fd_set
		for (int listnum = 0; listnum < MAX_CONNECTIONS; listnum++)
		{
			if (connectlist[listnum] != 0)
			{
				FD_SET(connectlist[listnum], &socks);
				if (connectlist[listnum] > highsock)
				{
					highsock = connectlist[listnum];
				}
			}
		}

		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		readsocks = select(highsock + 1, &socks, (fd_set *) 0, (fd_set *) 0,
				&timeout);
		if (readsocks < 0)
		{
			perror("select");
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
			for (int listnum = 0; listnum < MAX_CONNECTIONS; listnum++)
			{
				// fprintf(stderr, "%d of %d, ",listnum,MAX_CONNECTIONS);
				if (FD_ISSET(connectlist[listnum], &socks))
				{
					C_Message* m = C_Node::recv(connectlist[listnum]);

					m->listnum() = listnum;
					// Enqueue the request
					pthread_mutex_lock(&incoming_queue_mutex);
					{
					    // fprintf(stderr, "Got the mutex\n");
					    incoming_queue.append(m);
					    pthread_cond_signal(&not_empty_incoming_queue_cond);
					}
					pthread_mutex_unlock(&incoming_queue_mutex);
				}
			}
		}
	}
	pthread_exit(NULL);
}

void C_Replica::handle_new_connection()
{
	int listnum;
	int connection;

	// There is a new connection coming in
	// We  try to find a spot for it in connectlist
	connection = accept(in_socket_for_clients, NULL, NULL);
	if (connection < 0)
	{
		perror("accept");
		exit(EXIT_FAILURE);
	}
	setnonblocking(connection);
	for (listnum = 0; (listnum < MAX_CONNECTIONS) && (connection != -1); listnum ++)
	{
		if (connectlist[listnum] == 0)
		{
			fprintf(stderr, "\nConnection accepted:   FD=%d; Slot=%d\n", connection,
			listnum);
			connectlist[listnum] = connection;
			return;
		}
	}
	fprintf(stderr, "\nNo room left for new client.\n");
	close(connection);
}
