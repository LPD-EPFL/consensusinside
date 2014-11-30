#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "PBFT_R_pbft_libbyz.h"
#include "PBFT_R_Request.h"
#include "PBFT_R_Reply.h"
#include "PBFT_R_Client.h"
#include "PBFT_R_Replica.h"
#include "PBFT_R_Statistics.h"
#include "PBFT_R_State_defs.h"

static void wait_chld(int sig)
{
	// Get rid of zombies created by sfs code.
	while (waitpid(-1, 0, WNOHANG) > 0)
		;
}

int PBFT_R_pbft_init_client(char *conf, char *conf_priv, char *host_name,
		short port)
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

	PBFT_R_Client* client = new PBFT_R_Client(config_file, config_priv_file, host_name, port);
	PBFT_R_node = client;
	return 0;
}

void PBFT_R_pbft_reset_client()
{
	((PBFT_R_Client*) PBFT_R_node)->reset();
}

int PBFT_R_pbft_alloc_request(Byz_req *req)
{
	PBFT_R_Request* request = new PBFT_R_Request((Request_id) 0);
	if (request == 0)
		return -1;

	int len;
	req->contents = request->store_command(len);
	req->size = len;
	req->opaque = (void*) request;
	return 0;
}

int PBFT_R_pbft_send_request(Byz_req *req, bool ro)
{
	PBFT_R_Request *request = (PBFT_R_Request *) req->opaque;
	request->request_id() = ((PBFT_R_Client*) PBFT_R_node)->get_rid();
	request->authenticate(req->size, ro);

	bool retval = ((PBFT_R_Client*) PBFT_R_node)->send_request(request);
	return (retval) ? 0 : -1;
}

int PBFT_R_pbft_recv_reply(Byz_rep *rep)
{
	PBFT_R_Reply *reply = ((PBFT_R_Client*) PBFT_R_node)->recv_reply();
	if (reply == NULL)
		return -1;
	if (reply->should_switch()) {
	    next_protocol_instance = reply->next_instance_id();
	    return -127;
	}
	rep->contents = reply->reply(rep->size);
	rep->opaque = reply;
	return 0;
}

int PBFT_R_pbft_invoke(Byz_req *req, Byz_rep *rep, bool ro)
{
	if (PBFT_R_pbft_send_request(req, ro) == -1)
		return -1;
	return PBFT_R_pbft_recv_reply(rep);
}

void PBFT_R_pbft_free_request(Byz_req *req)
{
	PBFT_R_Request *request = (PBFT_R_Request *) req->opaque;
	delete request;
}

void PBFT_R_pbft_free_reply(Byz_rep *rep)
{
	PBFT_R_Reply *reply = (PBFT_R_Reply *) rep->opaque;
	delete reply;
}

#ifndef NO_STATE_TRANSLATION

int PBFT_R_pbft_init_replica(char *conf, char *conf_priv, unsigned int num_objs,
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

	PBFT_R_replica = new PBFT_R_Replica(config_file, config_priv_file, num_objs,
			get, put, shutdown_proc, restart_proc, port);
	node = PBFT_R_replica;

	// Register service-specific functions.
	if (getenv("PBFT_J2EE") == NULL) 
		PBFT_R_replica->registePBFT_R_exec(exec);
	else
		PBFT_R_replica->registePBFT_R_exec();
	PBFT_R_replica->registePBFT_R_nondet_choices(comp_ndet, ndet_max_len, check_ndet);
	return PBFT_R_replica->used_state_pages();
}

void PBFT_R_pbft_modify(int npages, int* pages)
{

	for (int i=0; i<npages; i++)
	PBFT_R_replica->modify_index(pages[i]);
}

#else

int PBFT_R_pbft_init_replica(char *conf, char *conf_priv, char *host_name,
		char *mem, unsigned int size, int(*exec)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool), void(*comp_ndet)(Seqno, Byz_buffer *), int ndet_max_len)
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

	fprintf(stderr, "PBFT_R_pbft_init_replica: progress 0 <%s>\n", host_name);
	PBFT_R_replica = new PBFT_R_Replica(config_file, config_priv_file, host_name, mem, size);
	PBFT_R_node = PBFT_R_replica;

	fprintf(stderr, "PBFT_R_pbft_init_replica: progress 1\n");
	// Register service-specific functions.
	if (getenv("PBFT_J2EE") == NULL)
		PBFT_R_replica->registePBFT_R_exec(exec);
	else
		PBFT_R_replica->registePBFT_R_exec();
	fprintf(stderr, "PBFT_R_pbft_init_replica: progress 2\n");
	PBFT_R_replica->registePBFT_R_nondet_choices(comp_ndet, ndet_max_len);
	fprintf(stderr, "PBFT_R_pbft_init_replica: progress 3\n");
	return PBFT_R_replica->used_state_bytes();
}

void PBFT_R_pbft_modify(char *mem, int size)
{
	PBFT_R_replica->modify(mem, size);
}

#endif

void PBFT_R_pbft_replica_run()
{
	stats.zero_stats();
	PBFT_R_replica->recv();
}

#ifdef NO_STATE_TRANSLATION
void PBFT_R__Byz_modify_index(int bindex)
{
	PBFT_R_replica->modify_index(bindex);
}
#endif

void PBFT_R_Byz_reset_stats()
{
	stats.zero_stats();
}

void PBFT_R_Byz_print_stats()
{
	stats.print_stats();
}

#ifndef NO_STATE_TRANSLATION
char* PBFT_R_Byz_get_cached_object(int i)
{
	return PBFT_R_replica->get_cached_obj(i);
}
#endif
