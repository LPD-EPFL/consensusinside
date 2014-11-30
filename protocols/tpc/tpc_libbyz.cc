#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#include "tpc_libbyz.h"

#include "T_Client.h"
#include "T_Replica.h"
#include "T_Request.h"
#include "T_Reply.h"

//#include "crypt.h"

//#define TRACES

T_Client *T_client;

void*T_replica_handler(void *o);
void*T_client_handler(void *o);
Byz_rep *T_srep;
static void wait_chld(int sig)
{
	// Get rid of zombies created by sfs code.
	while (waitpid(-1, 0, WNOHANG) > 0)
		;
}

int tpc_alloc_request(Byz_req *req)
{
	//fprintf(stderr,"Allocating new request\n");
	T_Request *request = new T_Request((Request_id) 0);
	// fprintf(stderr,"Allocated new request\n");
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

void tpc_free_request(Byz_req *req)
{
	T_Request *request = (T_Request *) req->opaque;
	delete request;
}

void tpc_free_reply(Byz_rep *rep)
{
	T_Reply *reply = (T_Reply *) rep->opaque;
	delete reply;
}

int tpc_init_replica(char *conf, char *conf_priv, char *host_name,
	int(*exec)(Byz_req*, Byz_rep*, Byz_buffer*, int, bool),
	int (*perform_checkpoint)(), short port)
{
	// signal handler to get rid of zombies
	struct sigaction act;
	act.sa_handler = wait_chld;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGCHLD, &act, NULL);

	// Initialize random number generator
	srand48(getpid());

	/*pthread_attr_t tattr;
	 int rc;
	 if(rc = pthread_attr_init(&tattr)){
	 fprintf(stderr,"ERROR: rc from pthread_attr_init() is %d\n",rc);
	 exit(-1);
	 }
	 if(rc = pthread_attr_setstacksize(&tattr,2*PTHREAD_STACK_MIN)){
	 fprintf(stderr,"ERROR: rc from pthread_attr_setstacksize() is %d\n",rc);
	 exit(-1);
	 }
	 */

	///////////////////////////
	// TPC
	///////////////////////////

	fprintf(stderr, "*******************************************\n");
	fprintf(stderr, "*        TPC replica protocol          *\n");
	fprintf(stderr, "*******************************************\n");

	FILE *config_file = fopen(conf, "r");
	if (config_file == 0)
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

	T_node = new T_Replica(config_file, config_priv_file, host_name, port);

	// Register service-specific functions.
	((T_Replica *) T_node)->register_exec(exec);

	fclose(config_file);
	fclose(config_priv_file);
	return 0;
}

int tpc_init_client(char *conf, char *host_name, short port)
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
	//random_init();

	fprintf(stderr, "******************************************\n");
	fprintf(stderr, "*         TPC client protocol         *\n");
	fprintf(stderr, "******************************************\n");

	FILE *config_file_tpc = fopen(conf, "r");
	if (config_file_tpc == 0)
	{
		fprintf(stderr, "libmodular_BFT: Invalid configuration file %s \n",
				conf);
		return -1;
	}

	T_client = new T_Client(config_file_tpc, host_name, port);

	/*  pthread_t T_client_handler_thread;
	 if (pthread_create(&T_client_handler_thread, 0, &T_client_handler,
	 (void *)T_client) != 0)
	 {
	 fprintf(stderr, "Failed to create the T_client thread\n");
	 exit(1);
	 }
	 */
	return 0;
}

int tpc_send_request(Byz_req *req, int size, bool ro)
{
	bool retval;
	T_Request *request = (T_Request *) req->opaque;
	retval = T_client->send_request(request, size, ro);
	return (retval) ? 0 : -1;
}

int tpc_invoke(Byz_req *req, Byz_rep *rep, int size, bool ro)
{
	if (tpc_send_request(req, size, ro) == -1)
	{
		return -1;
	}
	//Abort_data *ad = NULL;
	int retval = tpc_recv_reply(rep);
	if ( retval == 0)
	{
		;
		return 0;
	}
	return retval;
}

int tpc_recv_reply(Byz_rep *rep)
{
	T_Reply *reply = T_client->recv_reply();
	if (reply == NULL)
	{
		return -1;
	}
	else
	{
		rep->contents = reply->reply(rep->size);
		rep->opaque = reply;
		return 0;
	}
	return 0;
}
