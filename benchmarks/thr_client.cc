#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <sys/param.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include "th_assert.h"
//#include "Timer.h"
#include "libmodular_BFT.h"

#include "benchmarks.h"

//Added by Maysam Yabandeh
#include <vector.h>
#ifdef LIBTASK
#include <task.h>
#endif

#ifdef LIBTASK
void taskmain(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
	char config_abstract[PATH_MAX];
	char config_backup_BFT[PATH_MAX];
	char config_chain[PATH_MAX];
	char config_priv[PATH_MAX];
	char config_priv_tmp[PATH_MAX];
	char host_name[MAXHOSTNAMELEN+1];
	char master_host_name[MAXHOSTNAMELEN+1];

	short port_quorum;
	short port_pbft;
	short port_chain;
	int num_bursts;
	int num_messages_per_burst;
	int request_size = -1;
	char request_sizes_str[PATH_MAX];

	ssize_t init_hist_size = 0;

	int argNb=1;

	strcpy(host_name, argv[argNb++]);
	strcpy(master_host_name, argv[argNb++]);
	port_quorum = atoi(argv[argNb++]);
	port_pbft = atoi(argv[argNb++]);
	port_chain = atoi(argv[argNb++]);
	num_bursts = atoi(argv[argNb++]);
	num_messages_per_burst = atoi(argv[argNb++]);
	strcpy(request_sizes_str, argv[argNb++]);
	strcpy(config_abstract, argv[argNb++]);
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
	"Host name = %s\nPort_quorum = %d\nPort_pbft = %d\nPort_chain = %d\nNb bursts = %d \nNb messages per burst = %d\nInit history size = %d\nRequest sizes = '%s'\nConfiguration_quorum file = %s\nConfiguration_pbft file = %s\nConfig_private_pbft directory = %s\n",
	host_name, port_quorum, port_pbft, port_chain, num_bursts, num_messages_per_burst, init_hist_size, request_sizes_str, config_abstract, config_backup_BFT, config_priv_tmp);
	fprintf(stderr, "********************************************\n\n");

	char hname[MAXHOSTNAMELEN];
	gethostname(hname, MAXHOSTNAMELEN);

	// Try to open default file
	sprintf(config_priv, "%s/%s", config_priv_tmp, hname);

	srand(0);
	// Initialize client
	MBFT_init_client(host_name, config_abstract, config_backup_BFT,
			config_chain, config_priv, port_quorum, port_pbft, port_chain);

	// TODO let the time to the client to send its key
	sleep(2);

	// for varying sizes, in this order
	int sizes_counter = 0;
	int sizes[10];
	char *tok = strtok(request_sizes_str, " ");
	while (tok != NULL) {
	    int nsize = atoi(tok);
	    sizes[sizes_counter++] = nsize;
	    tok = strtok(NULL, " ");
	}

	// Allocate request
	int size = 8192;
	Byz_req req;
	MBFT_alloc_request(&req);

	th_assert(size <= req.size, "Request too big");

	//req.size = request_size;
	Byz_rep rep;

	// Create socket to communicate with manager
	int manager;
	if ((manager = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		th_fail("Could not create socket");
	}

	Address a;
	bzero((char *)&a, sizeof(a));
	a.sin_addr.s_addr = INADDR_ANY;
	a.sin_family = AF_INET;
	a.sin_port = htons(port_quorum+500);

	// Fill-in manager address
	Address desta;
	bzero((char *)&desta, sizeof(desta));

	struct hostent *hent = gethostbyname(master_host_name);
	if (hent == 0)
	{
		th_fail("Could not get hostent");
	}
	// desta.sin_addr.s_addr = inet_addr("192.168.20.6"); // sci6
	desta.sin_addr.s_addr = ((struct in_addr*)hent->h_addr_list[0])->s_addr;
	desta.sin_family = AF_INET;
	desta.sin_port = htons(MANAGER_PORT);
	if (connect(manager, (struct sockaddr *) &desta, sizeof(desta)) == -1)
	{
		th_fail("Could not connect name to socket");
	}

	thr_command out, in;

	// Tell manager we are up
	if (send(manager, &out, sizeof(out), 0) < sizeof(out))
	{
		fprintf(stderr, "Problem with send to manager\n");
		exit(-1);
	}

	fprintf(stderr, "Starting the bursts (num_bursts = %d)\n", num_bursts);

	fprintf(stderr, "Initializing INIT history %zu\n", init_hist_size);

	for (int j = 0; j < init_hist_size; j++)
	{
	    fprintf(stderr, " %d:", j);
	    MBFT_invoke(&req, &rep, request_size, false);
	    MBFT_free_reply(&rep);
	}
	fprintf(stderr,"\n");
	int i;
	int successful = 0;
	struct timeval totalrt;
	struct timeval begint;
	struct timeval endt;

	std::vector<struct timeval> beginTimes;
	std::vector<struct timeval> endTimes;

	for (i = 0; i < num_bursts; i++)
	{
		char *data = (char*)&in;
		int ssize = sizeof(in);
		int ret = 0;
		while (ssize) {
		    ret = recv(manager, data, ssize, MSG_WAITALL);
		    if (ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
			continue;
		    } else if (ret < 0) {
			fprintf(stderr, "Error receiving msg from manager\n");
			perror(NULL);
			exit(1);
		    }
		    ssize -= ret;
		    data += ret;
		}
		fprintf(stderr, "Starting burst #%d\n", i);
		//char a=getchar();
		// Loop invoking requests
		// int j=0;
		// while(true){
		//  j++
		int s = 0;
		s = in.num_iter;
		int j;
		successful = 0;
		totalrt.tv_sec = 0;
		totalrt.tv_usec = 0;
		for (j = s; j < num_messages_per_burst; j++)
		{
//			fprintf(stderr, "Send a new message\n");
			int retval = 0;
			request_size = sizes[j%sizes_counter];

			gettimeofday(&begint, NULL);
			retval = MBFT_invoke(&req, &rep, request_size, false);
			gettimeofday(&endt, NULL);
			//Added by Maysam Yabandeh
			beginTimes.push_back(begint);
			endTimes.push_back(endt);

			if (retval == -1)
			{
			    fprintf(stderr, "thr_client: problem invoking request\n");
			}
			else if (retval == -127)
			{
			    // fprintf(stderr, "thr_client: will switch to new protocol at message %d\n", j);
			    j--;
			    MBFT_alloc_request(&req);
			} else {
//			fprintf(stderr, "Get a reply\n");
			    // after receiving one message, we should switch back to quorum
			    if (j%1000 == 0)
			    	fprintf(stderr, ".");
			    MBFT_free_reply(&rep);
			    successful++;
			    totalrt.tv_sec += endt.tv_sec - begint.tv_sec;
			    totalrt.tv_usec += endt.tv_usec - begint.tv_usec;
			    while (totalrt.tv_usec < 0) {
				totalrt.tv_usec += 1000000;
				totalrt.tv_sec -= 1;
			    }
			    while (totalrt.tv_usec > 1000000) {
				totalrt.tv_usec -= 1000000;
				totalrt.tv_sec += 1;
			    }
			}
		}
		fprintf(stderr,"\n");
		//
		out.num_iter = successful;
		out.avg_rt = (totalrt.tv_sec + totalrt.tv_usec/1e9)/successful;
		if (send(manager, &out, sizeof(out), 0) <= 0)
		{
			fprintf(stderr, "Sendto failed");
			exit(-1);
		}
	}
	MBFT_free_request(&req);
	//Added by Maysam Yabandeh
#ifdef LIBTASK
	sleep(5);//because of poll error
#endif
	shutdown(manager, SHUT_RDWR);
	close(manager);
	fprintf(stderr, "Client exiting\n");
	//Added by Maysam Yabandeh
	//print the times into a file for later analysis
	char fileName[256];
	sprintf(fileName, "/tmp/client-%d-StartTimes.txt", port_quorum);
	FILE* startTimesFile = fopen(fileName, "a+");
	for (std::vector<struct timeval>::iterator i = beginTimes.begin(); i != beginTimes.end(); i++) 
		fprintf(startTimesFile, "%d %06d\n", i->tv_sec, i->tv_usec); 
	fclose(startTimesFile);
	sprintf(fileName, "/tmp/client-%d-endTimes.txt", port_quorum);
	FILE* endTimesFile = fopen(fileName, "a+");
	for (std::vector<struct timeval>::iterator i = endTimes.begin(); i != endTimes.end(); i++) 
		fprintf(endTimesFile, "%d %06d\n", i->tv_sec, i->tv_usec); 
	fclose(endTimesFile);
	//Added by Maysam Yabandeh
#ifdef LIBTASK
	taskexitall(0);
#endif
}

