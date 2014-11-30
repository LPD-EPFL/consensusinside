#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "crypt.h"
#include "rabin.h"

#include "th_assert.h"
#include "parameters.h"
#include "R_Message.h"
#include "R_Request.h"
#include "R_Principal.h"
#include "R_Node.h"
#include "pthread.h"

#define max(a,b) (a>=b?a:b)
#define min(a,b) (a<=b?a:b)

// Pointer to global R_node instance.
R_Node *R_node = 0;
R_Principal *R_my_principal = 0;

R_Node::R_Node(FILE *config_file, FILE *config_priv, char *host_name, short req_port) :
	incoming_queue()
{
	R_node = this;

	// read max_faulty and compute derived variables
	fscanf(config_file, "%d\n", &max_faulty);
	num_replicas = 3 * max_faulty + 1;
	if (num_replicas > Max_num_replicas)
	{
		th_fail("Invalid number of replicas");
	}

	// read in all the principals
	char addr_buff[257];

	if (fscanf(config_file, "%d\n", &num_principals) != 1) {
	    th_fail("Could not read number of principals");
	}
	if (num_replicas > num_principals)
	{
		th_fail("Invalid argument");
	}

	// read the multicast address
	short port;
	fscanf(config_file, "%256s %hd\n", addr_buff, &port);
	Addr a;
	bzero((char*)&a, sizeof(a));
	a.sin_family = AF_INET;
	a.sin_addr.s_addr = inet_addr(addr_buff);
	a.sin_port = htons(port);
	group = new R_Principal(num_principals + 1, num_principals,  a, a, NULL);


	if (config_priv != NULL) {
	    // Read private configuration file:
	    char pk1[1024], pk2[1024];
	    fscanf(config_priv, "%s %s\n", pk1, pk2);
	    bigint n1(pk1, 16);
	    bigint n2(pk2, 16);
	    if (n1 >= n2)
		th_fail("Invalid private file: first number >= second number");

	    priv_key = new rabin_priv(n1, n2);
	} else {
	    fprintf(stderr, "R_Node: Will not init priv_key\n");
	    priv_key = NULL;
	}

	// read in remaining principals' addresses and figure out my principal
	//char host_name[MAXHOSTNAMELEN+1];
	/*	if (gethostname(host_name, MAXHOSTNAMELEN))
	 {
	 perror("Unable to get hostname");
	 exit(1);
	 }
	 */
	fprintf(stderr, "R_Node: hostname = %s\n", host_name);
	struct hostent *hent = gethostbyname(host_name);
	if (hent == 0)
	{
		th_fail("Could not get hostent");
	}
	struct in_addr my_address = *((in_addr*)hent->h_addr_list[0]);
	node_id = -1;

	principals = (R_Principal**)malloc(num_principals*sizeof(R_Principal*));

	Addr TR_TCP_a;
	bzero((char*)&TR_TCP_a, sizeof(TR_TCP_a));
	TR_TCP_a.sin_family = AF_INET;
	Addr TR_TCP_a_for_clients;
	bzero((char*)&TR_TCP_a_for_clients, sizeof(TR_TCP_a_for_clients));
	TR_TCP_a_for_clients.sin_family = AF_INET;
	short TR_TCP_port;
	short TR_TCP_port_for_clients;

	long int fconf_pos = ftell(config_file);
	for (int i=0; i < num_principals; i++)
	{
		char hn[MAXHOSTNAMELEN+1];
		fscanf(config_file, "%256s %32s %hd %hd \n", hn, addr_buff,
				&TR_TCP_port, &TR_TCP_port_for_clients);
		TR_TCP_a.sin_addr.s_addr = inet_addr(addr_buff);
		TR_TCP_a.sin_port = htons(TR_TCP_port);
		TR_TCP_a_for_clients.sin_addr.s_addr = inet_addr(addr_buff);
		TR_TCP_a_for_clients.sin_port = htons(TR_TCP_port_for_clients);

		if (my_address.s_addr == TR_TCP_a.sin_addr.s_addr && node_id == -1
				&& (req_port == 0 || req_port == TR_TCP_port))
		{
			node_id = i;
			fprintf(stderr, "We have parsed entry %s and assigned node_id = %d\n", hn, node_id);
		}
	}
	fseek(config_file, fconf_pos, SEEK_SET);
	for (int i=0; i < num_principals; i++)
	{
		char hn[MAXHOSTNAMELEN+1];
		fscanf(config_file, "%256s %32s %hd %hd \n", hn, addr_buff,
				&TR_TCP_port, &TR_TCP_port_for_clients);
		TR_TCP_a.sin_addr.s_addr = inet_addr(addr_buff);
		TR_TCP_a.sin_port = htons(TR_TCP_port);
		TR_TCP_a_for_clients.sin_addr.s_addr = inet_addr(addr_buff);
		TR_TCP_a_for_clients.sin_port = htons(TR_TCP_port_for_clients);

		principals[i] = new R_Principal(i, num_principals, TR_TCP_a, TR_TCP_a_for_clients);
		if (node_id == i)
		{
			R_my_principal = principals[i];
		}
	}

	if (node_id < 0)
	{
		th_fail("R_Node: could not find my principal");
	}

	sleep(2);

	// Compute new timestamp for cur_rid
	new_tstamp();
	pthread_mutex_init(&incoming_queue_mutex, NULL);
	pthread_cond_init(&not_empty_incoming_queue_cond, NULL) ;

}

R_Node::~R_Node()
{
	for (int i=0; i < num_principals; i++)
	{
		delete principals[i];
	}

	free(principals);
}

void R_Node::send(R_Message *m, int socket)
{
	int size = m->size();
	send_all(socket, m->contents(), &size);
}

R_Message* R_Node::recv(int socket, int *status)
{
	R_Message* m = new R_Message(R_Max_message_size);

	int err = recv_all_blocking(socket, (char *)m->contents(), sizeof(R_Message_rep), 0);

	if (err == -1) {
	    delete m;
	    if (status != NULL)
		*status = err;
	    return NULL;
	}
	if (err == 0) {
	    // this means recv returned 0, which is a signal for closed connection
	    delete m;
	    if (status != NULL)
		*status = err;
	    return NULL;
	}
	int payload_size = m->size() - sizeof(R_Message_rep);
	if (payload_size < 0) {
	    fprintf(stderr, "R_Node[%d]: weird packet received with size < R_Message_rep\n", id());
	    delete m;
	    if (status != NULL)
		*status = err;
	    return NULL;
	}
	//fprintf(stderr, "Waiting for payload %d for tag %d\n", payload_size, m->tag());
	err = recv_all_blocking(socket, m->contents() + sizeof(R_Message_rep),
			payload_size, 0);
	if (err == -1) {
	    delete m;
	    if (status != NULL)
		*status = err;
	    return NULL;
	}
	if (status != NULL)
	    *status = err;
	return m;
}

void R_Node::gen_auth(char *s, unsigned l, char *dest, R_Reply_rep *rr,
		int principal_id, int entry_replica, int cid, int iteration, int skip) const
{
	long long unonce;
	if (skip == 0) {
	    unonce = R_Principal::new_umac_nonce();

	    memcpy(dest, (char*)&unonce, R_UNonce_size);
	} else {
	    memcpy((char*)&unonce, dest, R_UNonce_size);
	}
	dest += R_UNonce_size;
#ifdef TRACES
	fprintf(stderr, "R_Node[%d]: gen auth for pid %d, cid %d, entry_replica %d, iteration %d, unonce %lld\n", id(), principal_id, cid, entry_replica, iteration, unonce);
#endif

	bool first = iteration == 1;
	bool second = iteration == 2;
	bool third = iteration == 3;
#ifndef REPLY_BY_PRIMARY
	int edistance = distance(principal_id, (entry_replica-1+num_replicas)%num_replicas);
#else
	int edistance = distance(principal_id, (entry_replica-1+num_replicas)%num_replicas);
	if (edistance == 0 && second)
	    edistance = num_replicas;
#endif
	if (first)
	    edistance = f()+1;
	if (principal_id < 0)
	    edistance = num_replicas;
	int nb_MACs_for_replicas= min((f()+1), edistance);
	//int nb_MACs_for_replicas= f()+1;
	int start = (principal_id + 1)%num_replicas;
	if (principal_id < 0)
	    start = entry_replica;
	for (int j = skip; j < nb_MACs_for_replicas; j++)
	{
#ifdef TRACES
		fprintf(stderr, "R_Node[%d]: generating MAC for node %d, unonce %lld\n", id(), (start+j)%num_replicas, unonce);
#endif
		R_my_principal->gen_mac(s, l, dest, (start + j)%num_replicas,
				(char*)&unonce);
		dest += R_UMAC_size;
	}

	if (nb_MACs_for_replicas < f() + 1 && rr != NULL)
	{
		// Need to sign for the client
		char *d = (char*)(&rr->digest);
		R_my_principal->gen_mac(d, sizeof(Digest), dest, cid,
				(char*)&unonce);
#ifdef TRACES
		fprintf(stderr, "R_Node[%d]: generating MAC for client %d, unonce %lld\n", id(), cid, unonce);
#endif
	}
}

int R_Node::authenticators_size() const
{
	return R_UNonce_size * (f()+1)
	    + R_UMAC_size * ((f()+1)*(f()+2))/2 + (f()+1)*sizeof(Seqno);
}

int R_Node::authenticators_size_after_verification() const
{
	return R_UNonce_size * (f())
	    + R_UMAC_size * ((f())*(f()+1))/2 + (f()+1)*sizeof(Seqno);
}

bool R_Node::verify_auth(int n_id, int entry_replica, int cid, char *s, unsigned l, char *dest, int *authentication_offset, R_Request *r, int iteration)
{
#ifdef TRACES
	fprintf(stderr, "R_Node[%d]: verifying auth for %d, cid %d, entry replica %d, req = %p, seqno = %lld, iteration = %d\n", id(), n_id, cid, entry_replica, r, (r?r->seqno():0), iteration);
#endif
	// Do not remove n_id for replacing it with id()
	// Indeed, R_Reply::verify() uses n_id
	bool first = (iteration == 1);
	bool second = (iteration == 2);
	bool third = (iteration == 3);
	long long unonce = 0;
	char *initial_dest = dest;

	int edistance = distance(entry_replica, n_id) + 1;
	if (!first || n_id >= n()) // means: not the first round or R_Reply verification
	    edistance = f()+1;
	int i= min(f() + 1, edistance);
	int remaining_size;
	int seqnopos = 0;
	if (n_id < R_node->n() && n_id >= 0)
	{
		// Checking a request
		remaining_size = R_UMAC_size * ((f() + 1) * (f() + 2)) / 2
				+ R_UNonce_size * (f() + 1);
	} else
	{
		// Checking a reply or request from client
		remaining_size = (f() + 1) * (R_UNonce_size + R_UMAC_size);
	}
	while (i> 0)
	{
		int p_id;
		if (first && n_id >= 0 && n_id < n() && distance(entry_replica,n_id)<i)
		    p_id = cid;
		else if (n_id >=0 && n_id < n()) {
		    p_id = (n_id-i+num_replicas)%num_replicas;
		} else {
		    p_id = (entry_replica-i+num_replicas)%num_replicas;
		}

		Seqno os;
		if (r != NULL) {
		    Seqno s = r->get_stored_seqno(seqnopos);
		    r->swap_seqno(s, &os);
		    int dist_from_entry = distance(entry_replica, n_id);
		    if (!first && dist_from_entry < f()+1) {
			if (distance(p_id, n_id) > dist_from_entry + 1)
			{
			    r->set_type(false);
			}
		    }
		    if (third)
			r->set_type(true);
#ifdef TRACES
		    fprintf(stderr, "R_Node[%d]: new seqno is %lld, from position %d\n", id(), s, seqnopos);
#endif
		}

#ifdef TRACES
		fprintf(stderr, "R_Node[%d]: will verify MAC from node %d, i=%d\n", id(), p_id, i);
#endif
		memcpy((char*)&unonce, dest, R_UNonce_size);
		if (!principals[p_id]->verify_mac(s, l, dest + R_UNonce_size,
				(char *)&unonce))
		{
			fprintf(stderr, "R_Node[%d]: unable to verify mac from node %d, unonce %lld (i=%d)\n", id(), p_id, unonce, i);
			return false;
		}
		//int distance = ((entry_replica+num_replicas-1)-n_id)%num_replicas;
		int fdistance = f()+1;
		if (n_id >= R_node->n() || n_id < 0)
		    fdistance = 1;
		//int nb_MACs_from_p= min((f() + 2 - i), (num_replicas - n_id + 1));
		int nb_MACs_from_p= min((f() + 2 - i), fdistance);
#ifdef TRACES
		fprintf(stderr, "R_Node[%d]: nb_MACs_from_p = %d\n", id(), nb_MACs_from_p);
#endif
		if (nb_MACs_from_p == 1)
		{
			remaining_size -= (R_UNonce_size + R_UMAC_size);
			memmove(dest, dest + R_UNonce_size + R_UMAC_size, remaining_size);
		} else
		{
			memmove(dest + R_UNonce_size, dest + R_UNonce_size + R_UMAC_size,
					remaining_size - (R_UNonce_size + R_UMAC_size));
			remaining_size -= (R_UNonce_size + (nb_MACs_from_p - 1)
					* R_UMAC_size);
			dest += (R_UNonce_size + (nb_MACs_from_p - 1) * R_UMAC_size);
		}
		if (r != NULL) {
		    r->swap_seqno(os, NULL);
		    if (nb_MACs_from_p == 1)
			r->roll_stored_seqno();
		    else
			seqnopos++;
		    int dist_from_entry = distance(entry_replica, n_id);
		    if (!first && dist_from_entry < f()+1) {
			if (distance(p_id, n_id) > dist_from_entry + 1)
			{
			    r->set_type(true);
			}
		    }
		}

		i--;
	}
	*authentication_offset = dest - initial_dest;
	return true;
}

// verify just client's authentication.
// NOTE! will return true if there is nothing to do.
bool R_Node::verify_client_auth(int n_id, int entry_replica, int cid, char *s, unsigned l, char *dest, int *authentication_offset, R_Request *r, int iteration)
{
    //fprintf(stderr, "R_Node[%d]: verifying client auth for %d, cid %d, entry replica %d, req = %p, seqno = %lld\n", id(), n_id, cid, entry_replica, r, (r?r->seqno():0));
	// Do not remove n_id for replacing it with id()
	// Indeed, R_Reply::verify() uses n_id
	bool first = (iteration == 1);
	bool second = (iteration == 2);
	bool third = (iteration == 3);
	long long unonce = 0;
	char *initial_dest = dest;

	int edistance = distance(entry_replica, n_id) + 1;
	if (!first || n_id >= n()) // means: not the first round or R_Reply verification
	    edistance = f()+1;
	if (!first || edistance > f()+1)
	{
	    *authentication_offset = 0;
	    return true; // just return true in this case...
	}

	int i= min(f() + 1, edistance);

	int remaining_size;
	int seqnopos = 0;

	// Checking a request
	remaining_size = R_UMAC_size * ((f() + 1) * (f() + 2)) / 2 + R_UNonce_size * (f() + 1);

	int p_id = cid;

	Seqno os;
	if (r != NULL) {
	    Seqno s = r->get_stored_seqno(seqnopos);
	    r->swap_seqno(s, &os);
	}

	memcpy((char*)&unonce, dest, R_UNonce_size);
	if (!principals[p_id]->verify_mac(s, l, dest + R_UNonce_size,
		    (char *)&unonce))
	{
	    fprintf(stderr, "R_Node[%d]: unable to verify mac from node %d, unonce %lld (i=%d)\n", id(), p_id, unonce, i);
	    return false;
	}
	int fdistance = 2;
	int nb_MACs_from_p= min((f() + 2 - i), fdistance);
	if (nb_MACs_from_p == 1)
	{
	    remaining_size -= (R_UNonce_size + R_UMAC_size);
	    memmove(dest, dest + R_UNonce_size + R_UMAC_size, remaining_size);
	} else
	{
	    memmove(dest + R_UNonce_size, dest + R_UNonce_size + R_UMAC_size,
		    remaining_size - (R_UNonce_size + R_UMAC_size));
	    remaining_size -= (R_UNonce_size + (nb_MACs_from_p - 1)
		    * R_UMAC_size);
	    dest += (R_UNonce_size + (nb_MACs_from_p - 1) * R_UMAC_size);
	}
	if (r != NULL) {
	    r->swap_seqno(os, NULL);
	    if (nb_MACs_from_p == 1)
		r->roll_stored_seqno();
	    else
		seqnopos++;
	}

	*authentication_offset = dest - initial_dest;
	return true;
}

// here, we know that we're authenticating the tuple
// <type, seqno, digest1, digest2, ....>
// skip represents how many initial principals we are skipping
// authentication offset is taken into account, and updated.
bool R_Node::verify_batched_auth(int n_id, int entry_replica, int cid, char *s, unsigned l, char *dest, int *authentication_offset, R_Request *r, int iteration, int skip)
{
    //fprintf(stderr, "R_Node[%d]: verifying batched auth for %d, cid %d, entry replica %d, req = %p, seqno = %lld\n", id(), n_id, cid, entry_replica, r, (r?r->seqno():0));
    //fprintf(stderr, "authentication offset is %d, skip is %d, iteration %d\n", *authentication_offset, skip, iteration);
	// Do not remove n_id for replacing it with id()
	// Indeed, R_Reply::verify() uses n_id
	bool first = (iteration == 1);
	bool second = (iteration == 2);
	bool third = (iteration == 3);
	long long unonce = 0;

	dest += *authentication_offset;
	char *initial_dest = dest;

	int edistance = distance(entry_replica, n_id) + 1;
	if (!first || n_id >= n()) // means: not the first round or R_Reply verification
	    edistance = f()+1;
	int i= min(f() + 1, edistance);
	i -= skip;
	int remaining_size;
	int seqnopos = 0;
	if (n_id < R_node->n() && n_id >= 0)
	{
		// Checking a request
		remaining_size = R_UMAC_size * ((f() + 1) * (f() + 2)) / 2
				+ R_UNonce_size * (f() + 1);
	} else
	{
		// Checking a reply or request from client
		remaining_size = (f() + 1) * (R_UNonce_size + R_UMAC_size);
	}
	while (i> 0)
	{
		int p_id;
		if (first && n_id >= 0 && n_id < n() && distance(entry_replica,n_id)<i)
		    p_id = cid;
		else if (n_id >=0 && n_id < n()) {
		    p_id = (n_id-i+num_replicas)%num_replicas;
		} else {
		    p_id = (entry_replica-i+num_replicas)%num_replicas;
		}

		Seqno os;
		if (r != NULL) {
		    Seqno seq = r->get_stored_seqno(seqnopos);
		    memcpy(s+sizeof(uint32_t), (char*)&seq, sizeof(Seqno)); // copy seqno, no need to swap
		    if (!first && distance(p_id,entry_replica)>=f()+1 && distance(entry_replica, n_id)<f()+1)
			bzero(s, sizeof(uint32_t));
		    if (third)
			*((uint32_t*)s) = 0xdeadbeef;
		}

		memcpy((char*)&unonce, dest, R_UNonce_size);
		if (!principals[p_id]->verify_mac(s, l, dest + R_UNonce_size,
				(char *)&unonce))
		{
			fprintf(stderr, "R_Node[%d]: unable to verify mac from node %d, unonce %lld (i=%d)\n", id(), p_id, unonce, i);
			return false;
		}
		int fdistance = 2;
		if (n_id >= R_node->n() || n_id < 0)
		    fdistance = 1;
		int nb_MACs_from_p= min((f() + 2 - i), fdistance);
		if (nb_MACs_from_p == 1)
		{
			remaining_size -= (R_UNonce_size + R_UMAC_size);
			memmove(dest, dest + R_UNonce_size + R_UMAC_size, remaining_size);
		} else
		{
			memmove(dest + R_UNonce_size, dest + R_UNonce_size + R_UMAC_size,
					remaining_size - (R_UNonce_size + R_UMAC_size));
			remaining_size -= (R_UNonce_size + (nb_MACs_from_p - 1)
					* R_UMAC_size);
			dest += (R_UNonce_size + (nb_MACs_from_p - 1) * R_UMAC_size);
		}
		if (r != NULL) {
		    if (nb_MACs_from_p == 1)
			r->roll_stored_seqno();
		    else
			seqnopos++;
		    if (!first && distance(p_id,entry_replica)>=f()+1 && distance(entry_replica, n_id)<f()+1)
			*((uint32_t*)s) = 0xdeadbeef;
		}

		i--;
	}
	*authentication_offset = dest - initial_dest;
	return true;
}

void R_Node::gen_signature(const char *src, unsigned src_len, char *sig)
{
	bigint bsig = priv_key->sign(str(src, src_len));
	int size = mpz_rawsize(&bsig);
	if (size + sizeof(unsigned) > sig_size())
		th_fail("Signature is too big");

	memcpy(sig, (char*) &size, sizeof(unsigned));
	sig += sizeof(unsigned);

	mpz_get_raw(sig, size, &bsig);
}

unsigned R_Node::decrypt(char *src, unsigned src_len, char *dst,
		unsigned dst_len)
{
	if (src_len < 2* sizeof(unsigned))
		return 0;

	bigint b;
	unsigned csize, psize;
	memcpy((char*)&psize, src, sizeof(unsigned));
	src += sizeof(unsigned);
	memcpy((char*)&csize, src, sizeof(unsigned));
	src += sizeof(unsigned);

	if (dst_len < psize || src_len < csize)
		return 0;

	mpz_set_raw(&b, src, csize);

	str ptext = priv_key->decrypt(b, psize);
	memcpy(dst, ptext.cstr(), ptext.len());

	return psize;
}

Request_id R_Node::new_rid()
{
	if ((unsigned)cur_rid == (unsigned)0xffffffff)
	{
		new_tstamp();
	}
	return ++cur_rid;
}

void R_Node::new_tstamp()
{
	struct timeval t;
	gettimeofday(&t, 0);
	th_assert(sizeof(t.tv_sec) <= sizeof(int), "tv_sec is too big");
	Long tstamp = t.tv_sec;
	long int_bits = sizeof(int)*8;
	cur_rid = tstamp << int_bits;
}

