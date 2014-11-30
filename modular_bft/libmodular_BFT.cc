#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

//#define RUN_TPC
//#define RUN_ONEACCEPTOR
#define RUN_MULTIPAXOS

#include "libmodular_BFT.h"
#ifdef RUN_ZLIGHT
#include "zlight_libbyz.h"
#endif
#ifdef RUN_QUORUM
#include "quorum_libbyz.h"
#endif
#ifdef RUN_CHAIN
#include "chain_libbyz.h"
#endif
#ifdef RUN_RING
#include "ring_libbyz.h"
#endif
#ifdef RUN_PBFT
#include "PBFT_R_pbft_libbyz.h"
#include "PBFT_C_pbft_libbyz.h"
#endif

#include "crypt.h"

#include "Switcher.h"

//Added by Maysam Yabandeh
#ifdef RUN_MULTIPAXOS
#include "multipaxos_libbyz.h"
#endif
#ifdef RUN_ONEACCEPTOR
#include "oneacceptor_libbyz.h"
#endif
#ifdef RUN_TPC
#include "tpc_libbyz.h"
#endif

//#define TRACES


#ifdef RUN_QUORUM
enum protocols_e current_protocol = quorum;
enum protocols_e next_protocol_instance =  quorum;
#endif
#ifdef RUN_TPC
enum protocols_e current_protocol = tpc;
enum protocols_e next_protocol_instance =  tpc;
#endif
#ifdef RUN_ONEACCEPTOR
enum protocols_e current_protocol = oneacceptor;
enum protocols_e next_protocol_instance =  oneacceptor;
#endif
#ifdef RUN_MULTIPAXOS
enum protocols_e current_protocol = multipaxos;
enum protocols_e next_protocol_instance =  multipaxos;
#endif
//#define RUN_CHAIN
//#undef RUN_RING
//#define RUN_QUORUM
#undef RUN_QUORUM
#undef RUN_RING
#undef RUN_CHAIN

#ifdef RUN_CHAIN
enum protocols_e current_protocol = chain;
enum protocols_e next_protocol_instance = chain;
#endif
#ifdef RUN_RING
enum protocols_e current_protocol = ring;
enum protocols_e next_protocol_instance = ring;
#endif

int switching_number = 0;

Switcher *great_switcher = NULL;

int (*exec_command)(Byz_req*, Byz_rep*, Byz_buffer*, int, bool);

// Service specific functions.
int _exec_command(Byz_req *inb, Byz_rep *outb, Byz_buffer *non_det, int client,
		bool ro)
{
	//		fprintf(stderr, "Client %d: [%c %c %c %c]\n", client, inb->contents[0], inb->contents[1],
	//				inb->contents[2], inb->contents[3]);
	if (inb->contents[0] == 'z' && inb->contents[1] == 'o' && inb->contents[2]
			== 'r' && inb->contents[3] == 'r' && inb->contents[4] == 'o')
	{
		fprintf(stderr, "This is a maintenance message from client %d\n", client);
	}
	exec_command(inb, outb, non_det, client, ro);
	return 0;
}

int nb_checkpoints = 0;
// Service specific functions.
#if defined(RUN_ONEACCEPTOR) || defined(RUN_MULTIPAXOS) || defined(RUN_TPC) 
int perform_checkpoint() {return 1;}
//TODO
//I changed it since i did not want to be dependednt o pbft
#else
int perform_checkpoint()
{
	nb_checkpoints++;
	if (nb_checkpoints % 100 == 0)
	{
		fprintf(stderr, "Perform checkpoint %d\n", nb_checkpoints);

		// Allocate request
		Byz_req req;
		if (PBFT_C_pbft_alloc_request(&req) != 0)
		{
			fprintf(stderr, "allocation failed");
			exit(-1);
		}
		//		fprintf(stderr, "request size = %d\n", req.size);
		req.size = 4096;

		req.contents[0] = 'z';
		req.contents[1] = 'o';
		req.contents[2] = 'r';
		req.contents[3] = 'r';
		req.contents[4] = 'o';

		Byz_rep rep;

		PBFT_C_pbft_invoke(&req, &rep, false);
		PBFT_C_pbft_free_reply(&rep);
		PBFT_C_pbft_free_request(&req);

	}
	return 0;
}
#endif

int MBFT_alloc_request(Byz_req *req)
{
	switch (current_protocol)
	{
#ifdef RUN_QUORUM
		case quorum:
		{
			return quorum_alloc_request(req);
		}
			break;
#endif
#ifdef RUN_TPC
		case tpc:
		{
			return tpc_alloc_request(req);
		}
			break;
#endif
#ifdef RUN_ONEACCEPTOR
		case oneacceptor:
		{
			return oneacceptor_alloc_request(req);
		}
			break;
#endif
#ifdef RUN_MULTIPAXOS
		case multipaxos:
		{
			return multipaxos_alloc_request(req);
		}
			break;
#endif
#ifdef RUN_RING
		case ring:
		{
			return ring_alloc_request(req);
		}
			break;
#endif
#ifdef RUN_CHAIN
		case chain:
		{
			return chain_alloc_request(req);
		}
			break;
#endif
#ifdef RUN_PBFT
		case pbft:
		{
			return PBFT_R_pbft_alloc_request(req);
		}
			break;
#endif
#ifdef RUN_ZLIGHT
		case zlight:
		{
			return zlight_alloc_request(req);
		}
			break;
#endif
		default:
		{
			fprintf(stderr, "MBFT_alloc_request: Unknown protocol\n");
		}
			break;
	}

	return 0;
}

void MBFT_free_request(Byz_req *req)
{
	switch (current_protocol)
	{
#ifdef RUN_QUORUM
		case quorum:
		{
			quorum_free_request(req);
		}
			break;
#endif
#ifdef RUN_TPC
		case tpc:
		{
			tpc_free_request(req);
		}
			break;
#endif
#ifdef RUN_ONEACCEPTOR
		case oneacceptor:
		{
			oneacceptor_free_request(req);
		}
			break;
#endif
#ifdef RUN_MULTIPAXOS
		case multipaxos:
		{
			multipaxos_free_request(req);
		}
			break;
#endif
#ifdef RUN_RING
		case ring:
		{
			ring_free_request(req);
		}
			break;
#endif
#ifdef RUN_CHAIN
		case chain:
		{
			chain_free_request(req);
		}
			break;
#endif
#ifdef RUN_PBFT
		case pbft:
		{
			PBFT_R_pbft_free_request(req);
		}
			break;
#endif
#ifdef RUN_ZLIGHT
		case zlight:
		{
			zlight_free_request(req);
		}
			break;
#endif			
		default:
		{
			fprintf(stderr, "MBFT_free_request: Unknown protocol\n");
		}
			break;
	}
}

void MBFT_free_reply(Byz_rep *rep)
{
	switch (current_protocol)
	{
#ifdef RUN_QUORUM
		case quorum:
		{
			quorum_free_reply(rep);
		}
			break;
#endif
#ifdef RUN_TPC
		case tpc:
		{
			tpc_free_reply(rep);
		}
			break;
#endif
#ifdef RUN_ONEACCEPTOR
		case oneacceptor:
		{
			oneacceptor_free_reply(rep);
		}
			break;
#endif
#ifdef RUN_MULTIPAXOS
		case multipaxos:
		{
			multipaxos_free_reply(rep);
		}
			break;
#endif
#ifdef RUN_RING
		case ring:
		{
			ring_free_reply(rep);
		}
			break;
#endif
#ifdef RUN_CHAIN
		case chain:
		{
			chain_free_reply(rep);
		}
			break;
#endif
#ifdef RUN_PBFT
		case pbft:
		{
			PBFT_R_pbft_free_reply(rep);
		}
			break;
#endif
#ifdef RUN_ZLIGHT
		case zlight:
		{
			zlight_free_reply(rep);
		}
			break;
#endif
		default:
		{
			fprintf(stderr, "MBFT_free_reply: Unknown protocol\n");
		}
			break;
	}
}

int MBFT_init_replica(char *host_name, char *conf_quorum, char *conf_pbft,
		char *conf_priv_pbft, char *conf_chain, char *mem,
		unsigned int mem_size, int(*exec)(
				Byz_req*, Byz_rep*, Byz_buffer*, int, bool), short port,
		short port_pbft, short port_chain)
{
    fprintf(stderr, "MBFT_init_replica called\n");
        great_switcher = new Switcher();

	exec_command = exec;

#ifdef RUN_ONEACCEPTOR
	oneacceptor_init_replica(conf_quorum, conf_priv_pbft, host_name, _exec_command, perform_checkpoint, port);
#endif
#ifdef RUN_MULTIPAXOS
	multipaxos_init_replica(conf_quorum, conf_priv_pbft, host_name, _exec_command, perform_checkpoint, port);
#endif
#ifdef RUN_TPC
	tpc_init_replica(conf_quorum, conf_priv_pbft, host_name, _exec_command, perform_checkpoint, port);
#endif
#ifdef RUN_QUORUM
	quorum_init_replica(conf_quorum, conf_priv_pbft, host_name, _exec_command, perform_checkpoint, port);
#endif
	//zlight_init_replica(conf_quorum, conf_priv_pbft, host_name, _exec_command, perform_checkpoint, port);
#ifdef RUN_CHAIN
	chain_init_replica(host_name, conf_chain, conf_priv_pbft, _exec_command, port_chain);
#endif
#ifdef RUN_RING
	ring_init_replica(host_name, conf_chain, conf_priv_pbft, _exec_command, port_chain);
#endif
	//PBFT_R_pbft_init_replica(conf_pbft, conf_priv_pbft, host_name, mem, mem_size, _exec_command, 0, 0);
	//PBFT_R_pbft_replica_run();
	fprintf(stderr, "PBFT_R_Replica is initialized\n");
	//PBFT_C_pbft_init_client(conf_pbft, conf_priv_pbft, 3000);
	//Added by Maysam Yabandeh - using task.h we should not loop anymore
#ifndef LIBTASK
	while (1)
	{
	 sleep(100);
	}
#endif
	return 0;
}

int MBFT_init_client(char *host_name, char *conf_quorum, char *conf_pbft,
		char *conf_chain, char *conf_priv_pbft, short port_quorum,
		short port_pbft, short port_chain)
{
#ifdef RUN_ONEACCEPTOR
	oneacceptor_init_client(conf_quorum, host_name, port_quorum);
#endif
#ifdef RUN_MULTIPAXOS
	multipaxos_init_client(conf_quorum, host_name, port_quorum);
#endif
#ifdef RUN_TPC
	tpc_init_client(conf_quorum, host_name, port_quorum);
#endif
#ifdef RUN_QUORUM
	quorum_init_client(conf_quorum, host_name, port_quorum);
#endif
	//zlight_init_client(conf_quorum, host_name, port_quorum);
#ifdef RUN_CHAIN
	chain_init_client(host_name, conf_chain, port_chain);
#endif
#ifdef RUN_RING
	ring_init_client(host_name, conf_chain, port_chain);
#endif
	//PBFT_R_pbft_init_client(conf_pbft, conf_priv_pbft, host_name, port_pbft);
	return 0;
}

int MBFT_invoke(Byz_req *req, Byz_rep *rep, int size, bool ro)
{
	int retval = 0;
	switch (current_protocol)
	{
#ifdef RUN_QUORUM
		case quorum:
		{
			retval = quorum_invoke(req, rep, size, ro);
		}
			break;
#endif
#ifdef RUN_TPC
		case tpc:
		{
			retval = tpc_invoke(req, rep, size, ro);
		}
			break;
#endif
#ifdef RUN_ONEACCEPTOR
		case oneacceptor:
		{
			retval = oneacceptor_invoke(req, rep, size, ro);
		}
			break;
#endif
#ifdef RUN_MULTIPAXOS
		case multipaxos:
		{
			retval = multipaxos_invoke(req, rep, size, ro);
		}
			break;
#endif
#ifdef RUN_RING
		case ring:
		{
			retval = ring_invoke(req, rep, size, ro);
		}
			break;
#endif
#ifdef RUN_CHAIN
		case chain:
		{
			retval = chain_invoke(req, rep, size, ro);
		}
			break;
#endif
#ifdef RUN_PBFT
		case pbft:
		{
			req->size = size;
			retval = PBFT_R_pbft_invoke(req, rep, ro);
		}
			break;
#endif
#ifdef RUN_ZLIGHT
		case zlight:
		{
			retval = zlight_invoke(req, rep, size, ro);
		}
			break;
#endif
		default:
		{
			fprintf(stderr, "MBFT_invoke: unknown protocol\n");
		}
			break;
	}
	if (retval == -127) {
	    // time to switch to another protocol
	    // fprintf(stderr, "MBFT_invoke: will switch to %d\n", next_protocol_instance);
	    MBFT_free_request(req);
	    req = NULL;
	    rep = NULL;
	    current_protocol = next_protocol_instance;
	}
	return retval;
}
