#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#include "quorum_libbyz.h"

#include "Q_Client.h"
#include "Q_Replica.h"
#include "Q_Request.h"
#include "Q_Reply.h"

//#include "crypt.h"

//#define TRACES

Q_Client *q_client;

void*Q_replica_handler(void *o);
void*Q_client_handler(void *o);
Byz_rep *srep;
static void wait_chld(int sig)
{
	// Get rid of zombies created by sfs code.
	while (waitpid(-1, 0, WNOHANG) > 0)
		;
}

int quorum_alloc_request(Byz_req *req)
{
	//fprintf(stderr,"Allocating new request\n");
	Q_Request *request = new Q_Request((Request_id) 0);
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

void quorum_free_request(Byz_req *req)
{
	Q_Request *request = (Q_Request *) req->opaque;
	delete request;
}

void quorum_free_reply(Byz_rep *rep)
{
	Q_Reply *reply = (Q_Reply *) rep->opaque;
	delete reply;
}

int quorum_init_replica(char *conf, char *conf_priv, char *host_name,
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
	// QUORUM
	///////////////////////////

	fprintf(stderr, "*******************************************\n");
	fprintf(stderr, "*        QUORUM replica protocol          *\n");
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

	Q_node = new Q_Replica(config_file, config_priv_file, host_name, port);

	// Register service-specific functions.
	((Q_Replica *) Q_node)->register_exec(exec);
	((Q_Replica *) Q_node)->register_perform_checkpoint(perform_checkpoint);

	fclose(config_file);
	fclose(config_priv_file);
	return 0;
}

int quorum_init_client(char *conf, char *host_name, short port)
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
	fprintf(stderr, "*         QUORUM client protocol         *\n");
	fprintf(stderr, "******************************************\n");

	FILE *config_file_quorum = fopen(conf, "r");
	if (config_file_quorum == 0)
	{
		fprintf(stderr, "libmodular_BFT: Invalid configuration file %s \n",
				conf);
		return -1;
	}

	q_client = new Q_Client(config_file_quorum, host_name, port);

	/*  pthread_t Q_client_handler_thread;
	 if (pthread_create(&Q_client_handler_thread, 0, &Q_client_handler,
	 (void *)q_client) != 0)
	 {
	 fprintf(stderr, "Failed to create the Q_client thread\n");
	 exit(1);
	 }
	 */
	return 0;
}

int quorum_send_request(Byz_req *req, int size, bool ro)
{
	bool retval;
	Q_Request *request = (Q_Request *) req->opaque;
	retval = q_client->send_request(request, size, ro);
	return (retval) ? 0 : -1;
}

int quorum_invoke(Byz_req *req, Byz_rep *rep, int size, bool ro)
{
	if (quorum_send_request(req, size, ro) == -1)
	{
		return -1;
	}
	//Abort_data *ad = NULL;
	int retval = quorum_recv_reply(rep);
	if ( retval == 0)
	{
		;
		return 0;
	}
	return retval;
}

int quorum_recv_reply(Byz_rep *rep)
{
	Q_Reply *reply = q_client->recv_reply();
	if (reply == NULL)
	{
		return -1;
	}
	else if (reply->should_switch())
	{
	    next_protocol_instance = reply->next_instance_id();
	    rep->opaque = NULL;
	    return -127;
	}
	else
	{
		rep->contents = reply->reply(rep->size);
		rep->opaque = reply;
		return 0;
	}
	return 0;
}
