#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#include "ring_libbyz.h"

#include "R_Client.h"
#include "R_Replica.h"
#include "R_Request.h"
#include "R_Reply.h"

#include "crypt.h"

//#define TRACES

R_Client *r_client;

void*R_replica_handler(void *o);
void*R_client_handler(void *o);
static void wait_chld(int sig)
{
	// Get rid of zombies created by sfs code.
	while (waitpid(-1, 0, WNOHANG) > 0)
		;
}

int ring_alloc_request(Byz_req *req)
{
	R_Request *request = new R_Request((Request_id) 0);
	if (request == 0)
	{
		return -1;
	}
	int len;
	req->contents = request->store_command(len);
	req->size = len;
	req->opaque = (void*) request;
	return 0;
}

void ring_free_request(Byz_req *req)
{
	R_Request *request = (R_Request *) req->opaque;
	delete request;
}

void ring_free_reply(Byz_rep *rep)
{
	R_Reply *reply = (R_Reply *) rep->opaque;
	delete reply;
}

int ring_init_replica(char *host_name, char *conf, char *conf_priv, int(*exec)(Byz_req*, Byz_rep*, Byz_buffer*,
		int, bool), short port)
{
	// signal handler to get rid of zombies
	struct sigaction act;
	act.sa_handler = wait_chld;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGCHLD, &act, NULL);

	// Initialize random number generator
	srand48(getpid());

	///////////////////////////
	// RING
	///////////////////////////

	fprintf(stderr, "*******************************************\n");
	fprintf(stderr, "*           RING replica protocol        *\n");
	fprintf(stderr, "*******************************************\n");

	FILE *config_file_ring = fopen(conf, "r");
	if (config_file_ring == 0)
	{
		fprintf(stderr, "libmodular_BFT: Invalid configuration file %s \n",
		conf);
		return -1;
	}

	FILE *config_priv_file = fopen(conf_priv, "r");
	if (config_priv_file == 0)
	{
		fprintf(stderr, "libmodular_BFT: Invalid private configuration file %s \n", conf_priv);
		return -1;
	}


	R_replica = new R_Replica(config_file_ring, config_priv_file, host_name, port);
	R_node = R_replica;

	// Register service-specific functions.
	R_replica->register_exec(exec);

	// Create receiving thread
	pthread_t R_replica_handler_thread;
	if (pthread_create(&R_replica_handler_thread, 0, &R_replica_handler,
			(void *) R_replica) != 0)
	{
		fprintf(stderr, "Failed to create the R_replica thread\n");
		exit(1);
	}

	//pthread_join(R_replica_handler_thread, NULL);
	return 0;
}

void*R_replica_handler(void *o)
{
	void **o2 = (void **) o;
	R_Replica &r = (R_Replica&) (*o2);

	r.do_recv_from_queue();
	/* while (1)
	 {
	 sleep(100000);
	 }*/
	// r.recv();
	return 0;
}

int ring_init_client(char *host_name, char *conf, short port)
{
	// signal handler to get rid of zombies
	struct sigaction act;
	act.sa_handler = wait_chld;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGCHLD, &act, NULL);

	// Initialize random number generator
	srand48(getpid());

	// Intialize random number generator
	random_init();

	fprintf(stderr, "******************************************\n");
	fprintf(stderr, "*           RING client protocol         *\n");
	fprintf(stderr, "******************************************\n");

	FILE *config_file_ring = fopen(conf, "r");
	if (config_file_ring == 0)
	{
		fprintf(stderr, "libmodular_BFT: Invalid configuration file %s \n",
		conf);
		return -1;
	}

	r_client = new R_Client(config_file_ring, host_name, port);

	return 0;
}

int ring_send_request(Byz_req *req, int size, bool ro)
{
	bool retval;
	R_Request *request = (R_Request *) req->opaque;
	request->request_id() = r_client->get_rid();
	request->set_replica((r_client->id()+0)%r_client->n());

	request->finalize(size, ro);
	request->authenticate(-1, 0, NULL, true);

	retval = r_client->send_request(request, size, ro);
	return (retval) ? 0 : -1;
}

int ring_invoke(Byz_req *req, Byz_rep *rep, int size, bool ro)
{
	if (ring_send_request(req, size, ro) == -1)
	{
		return -1;
	}
	//Abort_data *ad = NULL;
	if (ring_recv_reply(rep) == 0)
	{
		;
		return 0;
	}
	return 0;
}

int ring_recv_reply(Byz_rep *rep)
{
	// fprintf(stderr, "Calling ring\n");
	R_Reply *reply = r_client->recv_reply();
	if (reply == NULL)
	{
		return -1;
	} else
	{
		rep->contents = reply->reply(rep->size);
		rep->opaque = reply;
		return 0;
	}
	return 0;
}

