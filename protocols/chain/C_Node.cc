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
#include "C_Message.h"
#include "C_Principal.h"
#include "C_Node.h"
#include "pthread.h"

#define max(a,b) (a>=b?a:b)
#define min(a,b) (a<=b?a:b)

// Pointer to global C_node instance.
C_Node *C_node = 0;
C_Principal *C_my_principal = 0;

C_Node::C_Node(FILE *config_file, FILE *config_priv, char *host_name, short req_port) :
	incoming_queue()
{
	C_node = this;

	// read max_faulty and compute derived variables
	fscanf(config_file, "%d \n", &max_faulty);
	//Added by Maysam Yabandeh
	fscanf(config_file, "%d \n", &num_replicas);
	//num_replicas = 3 * max_faulty + 1;
	if (num_replicas > Max_num_replicas)
	{
		th_fail("Invalid number of replicas");
	}

	// read in all the principals
	char addr_buff[100];

	fscanf(config_file, "%d\n", &num_principals);
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
	group = new C_Principal(num_principals + 1, num_principals,  a, a, NULL);


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
	    fprintf(stderr, "C_Node: Will not init priv_key\n");
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
	fprintf(stderr, "C_Node: hostname = %s\n", host_name);
	struct hostent *hent = gethostbyname(host_name);
	if (hent == 0)
	{
		th_fail("Could not get hostent");
	}
	struct in_addr my_address = *((in_addr*)hent->h_addr_list[0]);
	node_id = -1;

	principals = (C_Principal**)malloc(num_principals*sizeof(C_Principal*));

	Addr TC_TCP_a;
	bzero((char*)&TC_TCP_a, sizeof(TC_TCP_a));
	TC_TCP_a.sin_family = AF_INET;
	Addr TC_TCP_a_for_clients;
	bzero((char*)&TC_TCP_a_for_clients, sizeof(TC_TCP_a_for_clients));
	TC_TCP_a_for_clients.sin_family = AF_INET;
	short TC_TCP_port;
	short TC_TCP_port_for_clients;

	long int fconf_pos = ftell(config_file);
	for (int i=0; i < num_principals; i++)
	{
		char hn[MAXHOSTNAMELEN+1];
		fscanf(config_file, "%256s %32s %hd %hd \n", hn, addr_buff,
				&TC_TCP_port, &TC_TCP_port_for_clients);
		TC_TCP_a.sin_addr.s_addr = inet_addr(addr_buff);
		TC_TCP_a.sin_port = htons(TC_TCP_port);
		TC_TCP_a_for_clients.sin_addr.s_addr = inet_addr(addr_buff);
		TC_TCP_a_for_clients.sin_port = htons(TC_TCP_port_for_clients);

		if (my_address.s_addr == TC_TCP_a.sin_addr.s_addr && node_id == -1
				&& (req_port == 0 || req_port == TC_TCP_port))
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
		    &TC_TCP_port, &TC_TCP_port_for_clients);
	    TC_TCP_a.sin_addr.s_addr = inet_addr(addr_buff);
	    TC_TCP_a.sin_port = htons(TC_TCP_port);
	    TC_TCP_a_for_clients.sin_addr.s_addr = inet_addr(addr_buff);
	    TC_TCP_a_for_clients.sin_port = htons(TC_TCP_port_for_clients);

	    principals[i] = new C_Principal(i, num_principals, TC_TCP_a, TC_TCP_a_for_clients);

	    if (node_id == i)
	    {
		C_my_principal = principals[i];
	    }
	}

	if (node_id < 0)
	{
		th_fail("C_Node: could not find my principal");
	}

	sleep(2);

	// Compute new timestamp for cur_rid
	new_tstamp();
	pthread_mutex_init(&incoming_queue_mutex, NULL);
	pthread_cond_init(&not_empty_incoming_queue_cond, NULL) ;

}

C_Node::~C_Node()
{
	for (int i=0; i < num_principals; i++)
	{
		delete principals[i];
	}

	free(principals);
}

void C_Node::send(C_Message *m, int socket)
{
	int size = m->size();
	send_all(socket, m->contents(), &size);
}

C_Message* C_Node::recv(int socket)
{
	C_Message* m = new C_Message(C_Max_message_size);

	recv_all_blocking(socket, (char *)m->contents(), sizeof(C_Message_rep),
			0);

	int payload_size = m->size() - sizeof(C_Message_rep);
	recv_all_blocking(socket, m->contents() + sizeof(C_Message_rep),
			payload_size, 0);
	return m;
}

void C_Node::gen_auth(char *s, unsigned l, char *dest, C_Reply_rep *rr,
		int principal_id, int cid) const
{
	long long unonce = C_Principal::new_umac_nonce();

	memcpy(dest, (char*)&unonce, C_UNonce_size);
	dest += C_UNonce_size;

	int nb_MACs_for_replicas= min((f()+1), (num_replicas - (principal_id + 1)));
	for (int j = 0; j < nb_MACs_for_replicas; j++)
	{
		C_my_principal->gen_mac(s, l, dest, principal_id + j + 1,
				(char*)&unonce);
		dest += C_UMAC_size;
	}
	if (nb_MACs_for_replicas < f() + 1)
	{
		// Need to sign for the client
		C_my_principal->gen_mac((char *)rr, sizeof(C_Reply_rep), dest, cid,
				(char*)&unonce);
	}
}

bool C_Node::verify_auth(int n_id, int cid, char *s, unsigned l, char *dest,
		int *authentication_offset) const
{
	// Do not remove n_id for replacing it with id()
	// Indeed, C_Reply::verify() uses n_id
	long long unonce = 0;
	char *initial_dest = dest;

	int i= min(f() + 1, n_id + 1);
	int remaining_size;
	if (n_id < C_node->n())
	{
		// Checking a request
		remaining_size = C_UMAC_size * ((f() + 1) * (f() + 2)) / 2
				+ C_UNonce_size * (f() + 1);
	} else
	{
		// Checking a reply
		remaining_size = (f() + 1) * (C_UNonce_size + C_UMAC_size);
	}
	while (i> 0)
	{
		int p_id = n_id - i;
		if (p_id < 0)
		{
			p_id = cid;
		}
		memcpy((char*)&unonce, dest, C_UNonce_size);
		if (!principals[p_id]->verify_mac(s, l, dest + C_UNonce_size,
				(char *)&unonce))
		{
			fprintf(stderr, "C_Replica %d: unable to verify mac from replica %d\n", id(), p_id);
			return false;
		}
		int nb_MACs_from_p= min((f() + 2 - i), (num_replicas - n_id + 1));
		if (nb_MACs_from_p == 1)
		{
			remaining_size -= (C_UNonce_size + C_UMAC_size);
			memmove(dest, dest + C_UNonce_size + C_UMAC_size, remaining_size);
		} else
		{
			memmove(dest + C_UNonce_size, dest + C_UNonce_size + C_UMAC_size,
					remaining_size - (C_UNonce_size + C_UMAC_size));
			remaining_size -= (C_UNonce_size + (nb_MACs_from_p - 1)
					* C_UMAC_size);
			dest += (C_UNonce_size + (nb_MACs_from_p - 1) * C_UMAC_size);
		}
		i--;
	}
	*authentication_offset = dest - initial_dest;
	return true;
}

void C_Node::gen_signature(const char *src, unsigned src_len, char *sig)
{
	bigint bsig = priv_key->sign(str(src, src_len));
	int size = mpz_rawsize(&bsig);
	if (size + sizeof(unsigned) > sig_size())
		th_fail("Signature is too big");

	memcpy(sig, (char*) &size, sizeof(unsigned));
	sig += sizeof(unsigned);

	mpz_get_raw(sig, size, &bsig);
}

unsigned C_Node::decrypt(char *src, unsigned src_len, char *dst,
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

Request_id C_Node::new_rid()
{
	if ((unsigned)cur_rid == (unsigned)0xffffffff)
	{
		new_tstamp();
	}
	return ++cur_rid;
}

void C_Node::new_tstamp()
{
	struct timeval t;
	gettimeofday(&t, 0);
	th_assert(sizeof(t.tv_sec) <= sizeof(int), "tv_sec is too big");
	Long tstamp = t.tv_sec;
	long int_bits = sizeof(int)*8;
	cur_rid = tstamp << int_bits;
}

