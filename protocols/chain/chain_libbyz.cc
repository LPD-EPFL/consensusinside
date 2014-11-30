#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#include "chain_libbyz.h"

#include "C_Client.h"
#include "C_Replica.h"
#include "C_Request.h"
#include "C_Reply.h"

#include "crypt.h"

//#define TRACES

C_Client *c_client;

void*C_replica_handler(void *o);
void*C_client_handler(void *o);
static void wait_chld(int sig)
{
	// Get rid of zombies created by sfs code.
	while (waitpid(-1, 0, WNOHANG) > 0)
		;
}

int chain_alloc_request(Byz_req *req)
{
	C_Request *request = new C_Request((Request_id) 0);
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

void chain_free_request(Byz_req *req)
{
	C_Request *request = (C_Request *) req->opaque;
	delete request;
}

void chain_free_reply(Byz_rep *rep)
{
	C_Reply *reply = (C_Reply *) rep->opaque;
	delete reply;
}

int chain_init_replica(char *host_name, char *conf, char *conf_priv, int(*exec)(Byz_req*, Byz_rep*, Byz_buffer*,
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
	// CHAIN
	///////////////////////////

	fprintf(stderr, "*******************************************\n");
	fprintf(stderr, "*           CHAIN replica protocol        *\n");
	fprintf(stderr, "*******************************************\n");

	FILE *config_file_chain = fopen(conf, "r");
	if (config_file_chain == 0)
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


	C_replica = new C_Replica(config_file_chain, config_priv_file, host_name, port);
	C_node = C_replica;

	// Register service-specific functions.
	C_replica->register_exec(exec);

	// Create receiving thread
	pthread_t C_replica_handler_thread;
	if (pthread_create(&C_replica_handler_thread, 0, &C_replica_handler,
			(void *) C_replica) != 0)
	{
		fprintf(stderr, "Failed to create the C_replica thread\n");
		exit(1);
	}

	//pthread_join(C_replica_handler_thread, NULL);
	return 0;
}

void*C_replica_handler(void *o)
{
	void **o2 = (void **) o;
	C_Replica &r = (C_Replica&) (*o2);

	r.do_recv_from_queue();
	/* while (1)
	 {
	 sleep(100000);
	 }*/
	// r.recv();
	return 0;
}

int chain_init_client(char *host_name, char *conf, short port)
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
	fprintf(stderr, "*           Chain client protocol         *\n");
	fprintf(stderr, "******************************************\n");

	FILE *config_file_chain = fopen(conf, "r");
	if (config_file_chain == 0)
	{
		fprintf(stderr, "libmodular_BFT: Invalid configuration file %s \n",
		conf);
		return -1;
	}

	c_client = new C_Client(config_file_chain, host_name, port);

	return 0;
}

int chain_send_request(Byz_req *req, int size, bool ro)
{
	bool retval;
	C_Request *request = (C_Request *) req->opaque;
	request->request_id() = c_client->get_rid();

	request->finalize(size, ro);
	request->authenticate(-1, 0, NULL);

	retval = c_client->send_request(request, size, ro);
	return (retval) ? 0 : -1;
}

int chain_invoke(Byz_req *req, Byz_rep *rep, int size, bool ro)
{
	if (chain_send_request(req, size, ro) == -1)
	{
		return -1;
	}
	//Abort_data *ad = NULL;
	if (chain_recv_reply(rep) == 0)
	{
		;
		return 0;
	}
	return 0;
}

int chain_recv_reply(Byz_rep *rep)
{
	// fprintf(stderr, "Calling chain\n");
	C_Reply *reply = c_client->recv_reply();
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

