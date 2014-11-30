#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/param.h>
#include <unistd.h>

#include "th_assert.h"
#include "libbyz.h"
#include "Timer.h"
#include "Array.h"

#include "benchmarks.h"
#include "incremental_stats.h"

class expdat { public: int pos; int cid; int len; int done; int notify; int make_switch; };

int main(int argc, char **argv)
{
	int num_clients;
	int num_bursts;
	int num_messages_per_burst;

	int argNb=1;

	num_clients = atoi(argv[argNb++]);
	num_bursts = atoi(argv[argNb++]);
	num_messages_per_burst = atoi(argv[argNb++]);

	// Priting parameters
	fprintf(stderr, "********************************************\n");
	fprintf(stderr, "*       Throughput bench parameters        *\n");
	fprintf(stderr, "********************************************\n");
	fprintf(stderr,
	"Nb clients = %d\nNb bursts = %d \nNb messages per burst = %d\n",
	num_clients, num_bursts, num_messages_per_burst);
	fprintf(stderr, "********************************************\n\n");

	Address client_addrs[num_clients];
	int clients;
	incr_stats throughput_stats;

	/* Create socket to communicate with clients */
	if ((clients = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		th_fail("Could not create socket");
	}

	Address a;
	bzero((char *)&a, sizeof(a));
	a.sin_addr.s_addr = INADDR_ANY;
	a.sin_family = AF_INET;
	a.sin_port = htons(MANAGER_PORT);
	if (bind(clients, (struct sockaddr *) &a, sizeof(a)) == -1)
	{
		th_fail("Could not bind name to socket");
	}

	thr_command out, in;
	int i;
	for (i=0; i < num_clients; i++)
	{
		size_t addr_len = sizeof(a);
		int ret = recvfrom(clients, &in, sizeof(in), 0, (struct sockaddr*)&a,
				&addr_len);
		if (ret != sizeof(in))
		{
			th_fail("Invalid message");
		}

		client_addrs[i] = a;
	}
	fprintf(stderr, "All ready\n");

	init_stats(&throughput_stats);

	Timer t;

	Array<expdat> experiment;

	FILE *fin = fopen("aliph.experiment", "r");
	th_assert(fin != NULL, "Problem opening file");

	while (!feof(fin)) {
	    expdat ed;
	    fscanf(fin, "%d %d %d %d %d %d", &ed.pos, &ed.cid, &ed.len, &ed.notify, &ed.done, &ed.make_switch);
	    experiment.append(ed);
	}
	fclose(fin);

	int cur_pos = 0;
	i = 0;
	t.reset();
	t.start();

	while (i < experiment.size())
	{
		fprintf(stderr, "thr_manager: cur_pos = %d\n", cur_pos);
		while (i < experiment.size() && cur_pos >= experiment[i].pos) {
		    expdat ed = experiment[i];
		    if (ed.cid >= num_clients) {
			i++;
			continue;
		    }
		    out.cid = ed.cid;
		    out.num_iter = ed.len;
		    out.proto = proto_quorum;
		    out.done = ed.done;
		    out.notify = ed.notify;
		    out.pos = ed.pos;
		    out.make_switch = ed.make_switch;

		    fprintf(stderr, "thr_manager: will start client %d[%d], len %d, notify %d, done %d\n", ed.cid, ed.cid == 0, ed.len, ed.notify, ed.done);
		    if (sendto(clients, &out, sizeof(out), 0,
				(struct sockaddr*)&(client_addrs[ed.cid]), sizeof(Address)) <= 0)
			{
				th_fail("Sendto");
			}
		    i++;
		}

		/* Wait for results */
		int ret = recvfrom(clients, &in, sizeof(in), 0, 0, 0);
		if (ret != sizeof(in))
		{
		    th_fail("Invalid message");
		}
		/*if (in.num_iter != num_messages_per_burst && in.num_iter != -1)
		{
		    th_fail("Invalid reply from client");
		    //        ret = recvfrom(clients, &in, sizeof(in), 0, 0, 0);

		}
		*/
		if (in.done == 0) {
		    cur_pos = in.pos >= cur_pos ? in.pos+1:cur_pos+1;
		    fprintf(stderr, "thr_manager: cur_pos is now %d\n", cur_pos);
		}
	}
	t.stop();

	//double throughput = ((double) (num_messages_per_burst)* num_c) / t.elapsed();

	//	add(&throughput_stats, throughput);

	//	fprintf(stderr, "Burst %d: throughput : %10.6g op/s avg = %10.6g std dev = %10.6g (elapsed time = %f)\n", i,
	//	throughput, get_avg(&throughput_stats),
	//	get_std_dev(&throughput_stats), t.elapsed());

		/* Separate out experiments to improve independence */
	//	sleep(1);

	sleep(10);
	int j = 0;
	while (j < num_clients)
        {
	    int ret = recvfrom(clients, &in, sizeof(in), 0, 0, 0);
	    if (ret != sizeof(in))
	    {
		th_fail("Invalid message");
	    }
	    if (in.num_iter != -1 && in.done != 1)
	    {
		th_fail("Invalid reply from client");
		//        ret = recvfrom(clients, &in, sizeof(in), 0, 0, 0);
	    }
	    j++;
	}
	fprintf(stderr, "thr_manager: exiting\n");
}

