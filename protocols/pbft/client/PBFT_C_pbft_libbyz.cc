#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "PBFT_C_pbft_libbyz.h"
#include "PBFT_C_Request.h"
#include "PBFT_C_Reply.h"
#include "PBFT_C_Client.h"
#include "PBFT_C_Replica.h"
#include "PBFT_C_Statistics.h"
#include "PBFT_C_State_defs.h"

static void wait_chld(int sig)
{
	// Get rid of zombies created by sfs code.
	while (waitpid(-1, 0, WNOHANG) > 0)
		;
}

int PBFT_C_pbft_init_client(char *conf, char *conf_priv, short port)
{
	// signal handler to get rid of zombies
	struct sigaction act;
	act.sa_handler = wait_chld;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGCHLD, &act, NULL);

	FILE *config_file = fopen(conf, "r");
	if (config_file == 0)
	{
		fprintf(stderr, "libbyz: Invalid configuration file %s \n", conf);
		return -1;
	}

	FILE *config_priv_file = fopen(conf_priv, "r");
	if (config_priv_file == 0)
	{
		fprintf(stderr, "libbyz: Invalid private configuration file %s \n",
				conf);
		return -1;
	}

	// Initialize random number generator
	srand48(getpid());

	PBFT_C_Client* client = new PBFT_C_Client(config_file, config_priv_file, port);
	PBFT_C_node = client;
	return 0;
}

void PBFT_C_pbft_reset_client()
{
	((PBFT_C_Client*) PBFT_C_node)->reset();
}

int PBFT_C_pbft_alloc_request(Byz_req *req)
{
	PBFT_C_Request* request = new PBFT_C_Request((Request_id) 0);
	if (request == 0)
		return -1;

	int len;
	req->contents = request->store_command(len);
	req->size = len;
	req->opaque = (void*) request;

	//fprintf(stderr, "has been allocated with size = %d\n", len);
	return 0;
}

int PBFT_C_pbft_send_request(Byz_req *req, bool ro)
{
	PBFT_C_Request *request = (PBFT_C_Request *) req->opaque;
	request->request_id() = ((PBFT_C_Client*) PBFT_C_node)->get_rid();
	request->authenticate(req->size, ro);

	bool retval = ((PBFT_C_Client*) PBFT_C_node)->send_request(request);
	return (retval) ? 0 : -1;
}

int PBFT_C_pbft_recv_reply(Byz_rep *rep)
{
	PBFT_C_Reply *reply = ((PBFT_C_Client*) PBFT_C_node)->recv_reply();
	if (reply == NULL)
		return -1;
	rep->contents = reply->reply(rep->size);
	rep->opaque = reply;
	return 0;
}

int PBFT_C_pbft_invoke(Byz_req *req, Byz_rep *rep, bool ro)
{
	if (PBFT_C_pbft_send_request(req, ro) == -1)
		return -1;
	return PBFT_C_pbft_recv_reply(rep);
}

void PBFT_C_pbft_free_request(Byz_req *req)
{
	PBFT_C_Request *request = (PBFT_C_Request *) req->opaque;
	delete request;
}

void PBFT_C_pbft_free_reply(Byz_rep *rep)
{
	PBFT_C_Reply *reply = (PBFT_C_Reply *) rep->opaque;
	delete reply;
}

#ifndef NO_STATE_TRANSLATION

int PBFT_C_pbft_init_replica(char *conf, char *conf_priv, unsigned int num_objs,
		int (*exec)(Byz_req *, Byz_rep *, Byz_buffer*, int, bool),
		void (*comp_ndet)(Seqno, Byz_buffer *), int ndet_max_len,
		bool (*check_ndet)(Byz_buffer *),
		int (*get)(int, char **),
		void (*put)(int, int *, int *, char **),
		void (*shutdown_proc)(FILE *o),
		void (*restart_proc)(FILE *i),
		short port)
{

	// signal handler to get rid of zombies
	struct sigaction act;
	act.sa_handler = wait_chld;
	sigemptyset (&act.sa_mask);
	act.sa_flags = 0;
	sigaction (SIGCHLD, &act, NULL);

	FILE *config_file = fopen(conf, "r");
	if (config_file == 0)
	{
		fprintf(stderr, "libbyz: Invalid configuration file %s \n", conf);
		return -1;
	}

	FILE *config_priv_file = fopen(conf_priv, "r");
	if (config_priv_file == 0)
	{
		fprintf(stderr, "libbyz: Invalid private configuration file %s \n", conf_priv);
		return -1;
	}

	// Initialize random number generator
	srand48(getpid());

	replica = new PBFT_C_Replica(config_file, config_priv_file, num_objs,
			get, put, shutdown_proc, restart_proc, port);
	node = replica;

	// Register service-specific functions.
	replica->register_exec(exec);
	replica->register_nondet_choices(comp_ndet, ndet_max_len, check_ndet);
	return replica->used_state_pages();
}

void PBFT_C_pbft_modify(int npages, int* pages)
{

	for (int i=0; i<npages; i++)
	replica->modify_index(pages[i]);
}

#else

int PBFT_C_pbft_init_replica(char *conf, char *conf_priv, char *mem, unsigned int size,
		int(*exec)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool),
		void(*comp_ndet)(Seqno, Byz_buffer *), int ndet_max_len)
{
	// signal handler to get rid of zombies
	struct sigaction act;
	act.sa_handler = wait_chld;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGCHLD, &act, NULL);

	FILE *config_file = fopen(conf, "r");
	if (config_file == 0)
	{
		fprintf(stderr, "libbyz: Invalid configuration file %s \n", conf);
		return -1;
	}

	FILE *config_priv_file = fopen(conf_priv, "r");
	if (config_priv_file == 0)
	{
		fprintf(stderr, "libbyz: Invalid private configuration file %s \n",
				conf_priv);
		return -1;
	}

	// Initialize random number generator
	srand48(getpid());

	PBFT_C_replica = new PBFT_C_Replica(config_file, config_priv_file, mem, size);
	PBFT_C_node = PBFT_C_replica;

	// Register service-specific functions.
	PBFT_C_replica->register_exec(exec);
	PBFT_C_replica->register_nondet_choices(comp_ndet, ndet_max_len);
	return PBFT_C_replica->used_state_bytes();
}

void PBFT_C_pbft_modify(char *mem, int size)
{
	PBFT_C_replica->modify(mem, size);
}

#endif

void PBFT_C_pbft_replica_run()
{
	PBFT_C_stats.zero_PBFT_C_stats();
	PBFT_C_replica->recv();
}

#ifdef NO_STATE_TRANSLATION
void PBFT_C__Byz_modify_index(int bindex)
{
	PBFT_C_replica->modify_index(bindex);
}
#endif

void PBFT_C_Byz_reset_PBFT_C_stats()
{
	PBFT_C_stats.zero_PBFT_C_stats();
}

void PBFT_C_Byz_print_PBFT_C_stats()
{
	PBFT_C_stats.print_PBFT_C_stats();
}

#ifndef NO_STATE_TRANSLATION
char* PBFT_C_Byz_get_cached_object(int i)
{
	return replica->get_cached_obj(i);
}
#endif
