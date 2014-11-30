#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <sys/param.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include <sys/time.h>
#include <sys/resource.h>

#include "th_assert.h"
//#include "Timer.h"
#include "libmodular_BFT_aliph.h"

#include "benchmarks.h"

enum bft_protocol current_proto = proto_quorum;
int valid = 0;

void *timerThread(void *arg) {
    struct timespec t;
    t.tv_nsec = 0;
    t.tv_sec = 2;
    nanosleep(&t, NULL);
    // code to execute
    if (valid) {
    	fprintf(stderr, "Will switch to backup, timeout\n");
    	current_proto = proto_backup;
    }
    return NULL;
}

int startTimer() {
    pthread_t tid;
    pthread_create(&tid, NULL, timerThread, NULL);
    return 0;
}

void printTimestamp(FILE *fp) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    fprintf(fp, "%u.%06u: ", tv.tv_sec, tv.tv_usec);
}

int main(int argc, char **argv)
{
	char config_quorum[PATH_MAX];
	char config_backup_BFT[PATH_MAX];
	char config_chain[PATH_MAX];
	char config_priv[PATH_MAX];
	char config_priv_tmp[PATH_MAX];
	char host_name[MAXHOSTNAMELEN+1];

	short port_quorum;
	short port_pbft;
	short port_chain;
	int num_bursts;
	int num_messages_per_burst;
	int request_size;

	ssize_t init_hist_size = 0;

	int argNb=1;

	strcpy(host_name, argv[argNb++]);
	port_quorum = atoi(argv[argNb++]);
	port_pbft = atoi(argv[argNb++]);
	port_chain = atoi(argv[argNb++]);
	num_bursts = atoi(argv[argNb++]);
	num_messages_per_burst = atoi(argv[argNb++]);
	request_size = atoi(argv[argNb++]);
	strcpy(config_quorum, argv[argNb++]);
	strcpy(config_backup_BFT, argv[argNb++]);
	strcpy(config_chain, argv[argNb++]);
	strcpy(config_priv_tmp, argv[argNb++]);

	if (argNb < argc) {
	    fprintf(stderr, "Will read last one\n");
	    init_hist_size = atol(argv[argNb++]);
	}

	// Priting parameters
	fprintf(stderr, "********************************************\n");
	fprintf(stderr, "*             Client parameters            *\n");
	fprintf(stderr, "********************************************\n");
	fprintf(stderr,
	"Host name = %s\nPort_quorum = %d\nPort_pbft = %d\nPort_chain = %d\nNb bursts = %d \nNb messages per burst = %d\nInit history size = %d\nRequest size = %d\nConfiguration_quorum file = %s\nConfiguration_pbft file = %s\nConfig_private_pbft directory = %s\n",
	host_name, port_quorum, port_pbft, port_chain, num_bursts, num_messages_per_burst, init_hist_size, request_size, config_quorum, config_backup_BFT, config_priv_tmp);
	fprintf(stderr, "********************************************\n\n");

	char hname[MAXHOSTNAMELEN];
	gethostname(hname, MAXHOSTNAMELEN);

	// Try to open default file
	sprintf(config_priv, "%s/%s", config_priv_tmp, hname);

	srand(0);
	// Initialize client
	MBFT_init_client_aliph(host_name, config_quorum,
			config_backup_BFT, config_chain, config_priv,
			port_quorum, port_pbft, port_chain);

	// TODO let the time to the client to send its key
	sleep(8);

	// initialize the log file
	char log_file_name[FILENAME_MAX];
	snprintf(log_file_name, MAXHOSTNAMELEN, "/tmp/client_%s.%d%d%d.log", hname, port_quorum, port_pbft, port_chain);
	FILE *log_file = fopen(log_file_name, "w+");
	th_assert(log_file != NULL, "Log file could not be opened\n");

	// Allocate request
	Byz_req chain_req;
	MBFT_alloc_request_chain(&chain_req);

	Byz_req quorum_req;
	MBFT_alloc_request_quorum(&quorum_req);

	Byz_req backup_req;
	MBFT_alloc_request_pbft(&backup_req);

	int size = 8192;
	th_assert(size <= chain_req.size, "Request too big");
	th_assert(size <= quorum_req.size, "Request too big");
	th_assert(size <= backup_req.size, "Request too big");

	//req.size = request_size;
	Byz_rep chain_rep;
	Byz_rep quorum_rep;
	Byz_rep backup_rep;

	// Create socket to communicate with manager
	int manager;
	if ((manager = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		th_fail("Could not create socket");
	}

	Address a;
	bzero((char *)&a, sizeof(a));
	a.sin_addr.s_addr = INADDR_ANY;
	a.sin_family = AF_INET;
	a.sin_port = htons(port_quorum+500);
	if (bind(manager, (struct sockaddr *) &a, sizeof(a)) == -1)
	{
		th_fail("Could not bind name to socket");
	}

	// Fill-in manager address
	Address desta;
	bzero((char *)&desta, sizeof(desta));
	// TODO change this: it should not be hard-coded
	desta.sin_addr.s_addr = inet_addr("192.168.20.6"); // sci6
	desta.sin_family = AF_INET;
	desta.sin_port = htons(MANAGER_PORT);

	thr_command out, in;

	// Tell manager we are up
	fprintf(stderr, "Notifying server that we're up\n");
	if (sendto(manager, &out, sizeof(out), 0, (struct sockaddr*)&desta,
			sizeof(desta)) <= 0)
	{
		th_fail();
	}

	setpriority(PRIO_PROCESS, 0, -10);
	/*
	fprintf(stderr, "Initializing INIT history %zu\n", init_hist_size);

	for (int j = 0; j < init_hist_size; j++)
	{
	    fprintf(stderr, " %d:", j);
	    MBFT_invoke_aliph(&req, &rep, request_size, false);
	    MBFT_free_reply(&rep);
	}
	fprintf(stderr,"\n");
	*/
	int tcounter = 0;
	struct timeval ttv;
	struct timeval btv;
	// replica gets info when to start
	// if in.num_iter == 0, that means replica needs to report every $msg_per_burst, for all bursts
	// else, in.num_iter represents number of iterations of $msg_per_burst client will do
	int sizes[] = { 2048, 512, 4096, 1024, 0 };
	const int sizes_counter = 5;

	int ret = recvfrom(manager, &in, sizeof(in), 0, 0, 0);
	if (ret != sizeof(in))
	{
	    fclose(log_file);
	    exit(-1);
	}

	while (!in.done) {
	    if (in.cid == 0) {
	    	valid = 1;
		    /*startTimer();*/
	    }
	    fprintf(stderr, "Client[%d]: Starting the burst (num messages = %d)\n", in.cid, in.num_iter);

	    current_proto = in.proto;
	    for (int j = 0; j < in.num_iter; j++)
	    {
		int retval = 0;
		request_size = sizes[j%sizes_counter];
		if (j%1000==0)
		    fprintf(stderr, ".");
		if (in.notify && in.notify == j) {
		    out.pos = in.pos;
		    out.done = 0;
		    if (sendto(manager, &out, sizeof(out), 0, (struct sockaddr*)&desta, sizeof(desta)) <= 0)
		    {
			exit(-1);
		    }
		    fprintf(stderr, "Will notify the manager\n");
		    printTimestamp(log_file);
		    fprintf(log_file, "NOTIFICATION SENT %d\n", j);
		}
		printTimestamp(log_file);
		fprintf(log_file, "INVOKE REQUEST %d cid=%d proto=%d size=%d\n", j, in.cid, current_proto, request_size);
		if (current_proto == proto_quorum)
		{
		    bool switchnow = (in.make_switch == 1)
			&& (j == 10);

		    retval = MBFT_invoke_quorum(&quorum_req, &quorum_rep, request_size, switchnow);
		    printTimestamp(log_file);
		    fprintf(log_file, "RECEIVED RESPONSE (%d) %d cid=%d, proto=%d\n", retval,  j, in.cid, current_proto);
		    if (retval != -127)
			MBFT_free_reply_quorum(&quorum_rep);
		}
		else if (current_proto == proto_chain)
		{
		    retval = MBFT_invoke_chain(&chain_req, &chain_rep, request_size, false);
		    printTimestamp(log_file);
		    fprintf(log_file, "RECEIVED RESPONSE (%d) %d cid=%d, proto=%d\n", retval,  j, in.cid, current_proto);
		    if (retval != -127)
			MBFT_free_reply_chain(&chain_rep);
		}
		else
		{
		    retval = MBFT_invoke_pbft(&backup_req, &backup_rep, request_size, false);
		    printTimestamp(log_file);
		    fprintf(log_file, "RECEIVED RESPONSE (%d) %d cid=%d, proto=%d\n", retval,  j, in.cid, current_proto);
		    if (retval != -127)
			MBFT_free_reply_pbft(&backup_rep);
		}
		//fflush(log_file);
		if (retval == -127) {
		    if (current_proto == proto_quorum)
			current_proto = proto_chain;
		    else if (current_proto == proto_chain)
			current_proto = proto_backup;
		    else if (current_proto == proto_zlight)
		    	current_proto = proto_backup;
		    else
			current_proto = proto_quorum;
		    fprintf(stderr, "Switched to %d\n", current_proto);
		}
		/*if (in.num_iter == 0 && m_counter == 5 && current_proto == proto_chain) {*/
		/*current_proto = proto_backup;*/
		/*}*/
	    }
	    fprintf(stderr, "\n");
	    valid = 0;
	    fflush(log_file);
	    //fprintf(stderr,"\n");
	    //
	    out.pos = in.pos;
	    out.done = 1;
	    out.num_iter = in.num_iter;
	    fprintf(stderr, "client[%d]: will notify manager\n", in.cid);
	    if (sendto(manager, &out, sizeof(out), 0, (struct sockaddr*)&desta, sizeof(desta)) <= 0)
	    {
		fclose(log_file);
		exit(-1);
	    }
	    ret = recvfrom(manager, &in, sizeof(in), 0, 0, 0);
	    if (ret != sizeof(in)) {
		fclose(log_file);
		exit(1);
	    }
	    fprintf(stderr, "client[%d]: got response %d from manager\n", in.cid, in.done);
	}
	fclose(log_file);
	out.done = 1;
	out.num_iter = -1;
	if (sendto(manager, &out, sizeof(out), 0, (struct sockaddr*)&desta, sizeof(desta)) <= 0)
	{
	    exit(-1);
	}
	MBFT_free_request_pbft(&backup_req);
	MBFT_free_request_chain(&chain_req);
	MBFT_free_request_quorum(&quorum_req);
	fprintf(stderr, "Client exiting\n");
	sleep(6);
}

