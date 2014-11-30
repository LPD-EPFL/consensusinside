#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#include "libmodular_BFT_aliph.h"
#include "quorum_libbyz.h"
#include "chain_libbyz.h"
#include "PBFT_R_pbft_libbyz.h"
#include "PBFT_C_pbft_libbyz.h"

#include "crypt.h"

#include "Switcher.h"

//#define TRACES


int (*exec_command_aliph)(Byz_req*, Byz_rep*, Byz_buffer*, int, bool);

// Service specific functions.
int _exec_command_aliph(Byz_req *inb, Byz_rep *outb, Byz_buffer *non_det,
		int client, bool ro)
{
	//		fprintf(stderr, "Client %d: [%c %c %c %c]\n", client, inb->contents[0], inb->contents[1],
	//				inb->contents[2], inb->contents[3]);
	if (inb->contents[0] == 'z' && inb->contents[1] == 'o' && inb->contents[2]
			== 'r' && inb->contents[3] == 'r' && inb->contents[4] == 'o')
	{
		fprintf(stderr, "This is a maintenance message from client %d\n", client);
	}
	exec_command_aliph(inb, outb, non_det, client, ro);
	return 0;
}

int perform_checkpoint_aliph()
{
	return 0;
}

int MBFT_alloc_request_quorum(Byz_req *req)
{
	return quorum_alloc_request(req);
}

int MBFT_alloc_request_chain(Byz_req *req)
{
	return chain_alloc_request(req);
}

int MBFT_alloc_request_pbft(Byz_req *req)
{
	return PBFT_R_pbft_alloc_request(req);
}

void MBFT_free_request_quorum(Byz_req *req)
{
	quorum_free_request(req);
}

void MBFT_free_request_chain(Byz_req *req)
{
	chain_free_request(req);
}

void MBFT_free_request_pbft(Byz_req *req)
{
	PBFT_R_pbft_free_request(req);
}

void MBFT_free_reply_quorum(Byz_rep *rep)
{
	quorum_free_reply(rep);
}

void MBFT_free_reply_chain(Byz_rep *rep)
{
	chain_free_reply(rep);
}

void MBFT_free_reply_pbft(Byz_rep *rep)
{
	PBFT_R_pbft_free_reply(rep);
}

int MBFT_init_replica_aliph(char *host_name,
		char *conf_quorum, char *conf_pbft, char *conf_priv_pbft,
		char *conf_chain, char *mem, unsigned int mem_size, int(*exec)(
				Byz_req*, Byz_rep*, Byz_buffer*, int, bool),
		short port_quorum, short port_pbft,
		short port_chain, ssize_t init_hist_size, int misses)
{
	great_switcher = new Switcher();

	exec_command_aliph = exec;

	quorum_init_replica(conf_quorum, conf_priv_pbft, host_name,
			_exec_command_aliph, perform_checkpoint_aliph, port_quorum);
	chain_init_replica(host_name, conf_chain, conf_priv_pbft,
			_exec_command_aliph, port_chain);
	PBFT_R_pbft_init_replica(conf_pbft, conf_priv_pbft, host_name, mem,
			mem_size, _exec_command_aliph, 0, 0);
	PBFT_R_pbft_replica_run();
	great_switcher->switch_state(quorum, true);
	great_switcher->switch_state(pbft, false);

	fprintf(stderr, "PBFT_R_Replica is initialized\n");
	//PBFT_C_pbft_init_client(conf_pbft, conf_priv_pbft, 3000);
	while (1)
	{
		sleep(100);
	}
	return 0;
}

int MBFT_init_client_aliph(char *host_name,
		char *conf_quorum, char *conf_pbft, char *conf_chain,
		char *conf_priv_pbft, short port_quorum,
		short port_pbft, short port_chain)
{
	quorum_init_client(conf_quorum, host_name, port_quorum);
	chain_init_client(host_name, conf_chain, port_chain);
	PBFT_R_pbft_init_client(conf_pbft, conf_priv_pbft, host_name, port_pbft);
	return 0;
}

int MBFT_invoke_quorum(Byz_req *req, Byz_rep *rep, int size, bool ro)
{
	req->size = size;
	return quorum_invoke(req, rep, size, ro);
}

int MBFT_invoke_chain(Byz_req *req, Byz_rep *rep, int size, bool ro)
{
	req->size = size;
	return chain_invoke(req, rep, size, ro);
}

int MBFT_invoke_pbft(Byz_req *req, Byz_rep *rep, int size, bool ro)
{
	req->size = size;
	return PBFT_R_pbft_invoke(req, rep, ro);
}

