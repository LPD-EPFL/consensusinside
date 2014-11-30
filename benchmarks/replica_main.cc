#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/param.h>
#include <unistd.h>
#include <time.h>

#include "th_assert.h"
#include "Traces.h"
#include "libmodular_BFT.h"

#include "benchmarks.h"

//Added by Maysam Yabandeh
#ifdef LIBTASK
#include <task.h>
#endif

int reply_size;
long sleep_time_ns;

// Service specific functions.
int exec_command(Byz_req *inb, Byz_rep *outb, Byz_buffer *non_det, int client,
		bool ro)
{
	//fprintf(stderr, "replica_main.cc: We are executing a request. The client is %d, size %d\n", client, reply_size);
	bzero(outb->contents, reply_size);
	if (reply_size > sizeof(int))
	    memcpy(outb->contents, &client, sizeof(client));

	outb->size = reply_size;

	// now, it is time to sleep.
	if (sleep_time_ns != 0) {
	    struct timespec tts;
	    tts.tv_sec = 0;
	    tts.tv_nsec = sleep_time_ns;

	    nanosleep(&tts, NULL);
	}
	return 0;
}

#ifdef LIBTASK
void taskmain(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
	short port_quorum;
	short port_pbft;
	short port_chain;
	char config_quorum[PATH_MAX];
	char config_pbft[PATH_MAX];
	char config_priv_pbft[PATH_MAX];
	char config_priv_tmp_pbft[PATH_MAX];
	char config_chain[PATH_MAX];
	char host_name[MAXHOSTNAMELEN+1];
	int argNb = 1;
	ssize_t init_history_size = 0;
	int percent_misses = 0;

	strcpy(host_name, argv[argNb++]);
	port_quorum = atoi(argv[argNb++]);
	port_pbft = atoi(argv[argNb++]);
	port_chain = atoi(argv[argNb++]);
	reply_size = atoi(argv[argNb++]);
	sleep_time_ns = atol(argv[argNb++]);
	strcpy(config_quorum, argv[argNb++]);
	strcpy(config_pbft, argv[argNb++]);
	strcpy(config_priv_tmp_pbft, argv[argNb++]);
	strcpy(config_chain, argv[argNb++]);
	init_history_size = atoi(argv[argNb++]);
	percent_misses= atoi(argv[argNb++]);

	// Priting parameters
	fprintf(stderr, "********************************************\n");
	fprintf(stderr, "*             Replica parameters           *\n");
	fprintf(stderr, "********************************************\n");
	fprintf(
	stderr,
	"Host name = %s\nPort_quorum = %d\nPort_pbft = %d\nPort_chain = %d\nReply size = %d \nConfiguration_quorum file = %s\nConfiguration_chain file = %s\nConfiguration_pbft file = %s\nConfig_private_pbft directory = %s\n",
	host_name, port_quorum, port_pbft, port_chain, reply_size,
	config_quorum, config_chain, config_pbft, config_priv_tmp_pbft);
	fprintf(stderr, "********************************************\n\n");

	char hname[MAXHOSTNAMELEN];
	gethostname(hname, MAXHOSTNAMELEN);
	// Try to open default file
	sprintf(config_priv_pbft, "%s/%s", config_priv_tmp_pbft, hname);

	int mem_size = 455* 8192;
	char *mem = (char*) valloc(mem_size);
	bzero(mem, mem_size);

	MBFT_init_replica(host_name, config_quorum, config_pbft,
			config_priv_pbft, config_chain, mem, mem_size, exec_command,
			port_quorum, port_pbft, port_chain);
}

