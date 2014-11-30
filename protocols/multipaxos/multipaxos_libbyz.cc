#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#include "multipaxos_libbyz.h"

#include "M_Client.h"
#include "M_Replica.h"
#include "M_Request.h"
#include "M_Reply.h"

//#include "crypt.h"

//#define TRACES

M_Client *M_client;

void*M_replica_handler(void *o);
void*M_client_handler(void *o);
Byz_rep *M_srep;
static void wait_chld(int sig)
{
	// Get rid of zombies created by sfs code.
	while (waitpid(-1, 0, WNOHANG) > 0)
		;
}

int multipaxos_alloc_request(Byz_req *req)
{
	//fprintf(stderr,"Allocating new request\n");
	M_Request *request = new M_Request((Request_id) 0);
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

void multipaxos_free_request(Byz_req *req)
{
	M_Request *request = (M_Request *) req->opaque;
	delete request;
}

void multipaxos_free_reply(Byz_rep *rep)
{
	M_Reply *reply = (M_Reply *) rep->opaque;
	delete reply;
}

int multipaxos_init_replica(char *conf, char *conf_priv, char *host_name,
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
	// MULTIPAXOS
	///////////////////////////

	fprintf(stderr, "*******************************************\n");
	fprintf(stderr, "*        MULTIPAXOS replica protocol          *\n");
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

	M_node = new M_Replica(config_file, config_priv_file, host_name, port);

	// Register service-specific functions.
	((M_Replica *) M_node)->register_exec(exec);

	fclose(config_file);
	fclose(config_priv_file);
	return 0;
}

int multipaxos_init_client(char *conf, char *host_name, short port)
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
	fprintf(stderr, "*         MULTIPAXOS client protocol         *\n");
	fprintf(stderr, "******************************************\n");

	FILE *config_file_oneacceptor = fopen(conf, "r");
	if (config_file_oneacceptor == 0)
	{
		fprintf(stderr, "libmodular_BFT: Invalid configuration file %s \n",
				conf);
		return -1;
	}

	M_client = new M_Client(config_file_oneacceptor, host_name, port);

	/*  pthread_t M_client_handler_thread;
	 if (pthread_create(&M_client_handler_thread, 0, &M_client_handler,
	 (void *)M_client) != 0)
	 {
	 fprintf(stderr, "Failed to create the M_client thread\n");
	 exit(1);
	 }
	 */
	return 0;
}

int multipaxos_send_request(Byz_req *req, int size, bool ro)
{
	bool retval;
	M_Request *request = (M_Request *) req->opaque;
	retval = M_client->send_request(request, size, ro);
	return (retval) ? 0 : -1;
}

int multipaxos_invoke(Byz_req *req, Byz_rep *rep, int size, bool ro)
{
	if (multipaxos_send_request(req, size, ro) == -1)
	{
		return -1;
	}
	//Abort_data *ad = NULL;
	int retval = multipaxos_recv_reply(rep);
	if ( retval == 0)
	{
		;
		return 0;
	}
	return retval;
}

int multipaxos_recv_reply(Byz_rep *rep)
{
	M_Reply *reply = M_client->recv_reply();
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
