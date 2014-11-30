#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <sys/param.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "th_assert.h"
//#include "Timer.h"
#include "libmodular_BFT_aliph.h"

#include "benchmarks.h"

int main(int argc, char **argv)
{
	char config_switching[PATH_MAX];
	char config_quorum[PATH_MAX];
	char config_backup_BFT[PATH_MAX];
	char config_chain[PATH_MAX];
	char config_priv[PATH_MAX];
	char config_priv_tmp[PATH_MAX];
	char host_name[MAXHOSTNAMELEN+1];

	short port_switching;
	short port_quorum;
	short port_pbft;
	short port_chain;
	int num_bursts;
	int num_messages_per_burst;
	int request_size;

	ssize_t init_hist_size = 0;

	int argNb=1;

	strcpy(host_name, argv[argNb++]);
	port_switching = atoi(argv[argNb++]);
	port_quorum = atoi(argv[argNb++]);
	port_pbft = atoi(argv[argNb++]);
	port_chain = atoi(argv[argNb++]);
	num_bursts = atoi(argv[argNb++]);
	num_messages_per_burst = atoi(argv[argNb++]);
	request_size = atoi(argv[argNb++]);
	strcpy(config_switching, argv[argNb++]);
	strcpy(config_quorum, argv[argNb++]);
	strcpy(config_backup_BFT, argv[argNb++]);
	strcpy(config_chain, argv[argNb++]);
	strcpy(config_priv_tmp, argv[argNb++]);

	if (argNb < argc)
	{
		fprintf(stderr, "Will read last one\n");
		init_hist_size = atol(argv[argNb++]);
	}

	// Priting parameters
	fprintf(stderr, "********************************************\n");
	fprintf(stderr, "*             Client parameters            *\n");
	fprintf(stderr, "********************************************\n");
	fprintf(stderr,
	"Host name = %s\nPort_switching = %d\nPort_quorum = %d\nPort_pbft = %d\nPort_chain = %d\nNb bursts = %d \nNb messages per burst = %d\nInit history size = %d\nRequest size = %d\nConfiguration_switching file = %s\nConfiguration_quorum file = %s\nConfiguration_pbft file = %s\nConfig_private_pbft directory = %s\n",
	host_name, port_switching, port_quorum, port_pbft, port_chain, num_bursts, num_messages_per_burst, init_hist_size, request_size, config_switching, config_quorum, config_backup_BFT, config_priv_tmp);
	fprintf(stderr, "********************************************\n\n");

	char hname[MAXHOSTNAMELEN];
	gethostname(hname, MAXHOSTNAMELEN);

	// Try to open default file
	sprintf(config_priv, "%s/%s", config_priv_tmp, hname);

	srand(0);
	// Initialize client
	MBFT_init_client_aliph(host_name, config_switching, config_quorum,
			config_backup_BFT, config_chain, config_priv, port_switching,
			port_quorum, port_pbft, port_chain);

	// TODO let the time to the client to send its key
	sleep(2);

	// Allocate request
	int size = 4096;
	Byz_req req_switching;
	MBFT_alloc_request_switching(&req_switching);

	th_assert(size <= req_switching.size, "Request too big");

	//req.size = request_size;
	Byz_rep rep_switching;

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
	desta.sin_addr.s_addr = inet_addr("192.168.20.8"); // sci1
	desta.sin_family = AF_INET;
	desta.sin_port = htons(MANAGER_PORT);

	thr_command out, in;

	// Tell manager we are up
	if (sendto(manager, &out, sizeof(out), 0, (struct sockaddr*)&desta,
			sizeof(desta)) <= 0)
	{
		exit(-1);
	}

	fprintf(stderr, "Starting the bursts (num_bursts = %d)\n", num_bursts);

	int i;
	for (i = 0; i < num_bursts; i++)
	{
		int ret = recvfrom(manager, &in, sizeof(in), 0, 0, 0);
		if (ret != sizeof(in))
		{
			exit(-1);
		}
		fprintf(stderr, "Starting burst #%d\n", i);
		for (int j = 0; j < num_messages_per_burst; j++)
		{
			MBFT_invoke_switching(&req_switching, &rep_switching, request_size,
					false);
			MBFT_free_reply_switching(&rep_switching);
		}
		fprintf(stderr,"\n");
		//
		out.num_iter = num_messages_per_burst;
		if (sendto(manager, &out, sizeof(out), 0, (struct sockaddr*)&desta,
				sizeof(desta)) <= 0)
		{
			exit(-1);
		}
	}
	MBFT_free_request(&req_switching);
}

