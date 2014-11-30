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

#include <limits.h>

#include "crypt.h"
#include "rabin.h"

#include "th_assert.h"
#include "parameters.h"
#include "PBFT_R_Message.h"
#include "PBFT_R_Time.h"
#include "PBFT_R_ITimer.h"
#include "PBFT_R_Principal.h"
#include "PBFT_R_New_key.h"
#include "PBFT_R_Node.h"

//#define NO_IP_MULTICAST

#ifndef NDEBUG
#define NDEBUG
#endif

// Pointer to global node instance.
PBFT_R_Node *PBFT_R_node = 0;

//Enable statistics
#include "PBFT_R_Statistics.h"

PBFT_R_Node::PBFT_R_Node(FILE *config_file, FILE *config_priv, char* host_name,
		short req_port)
{
	PBFT_R_node = this;

#ifdef NO_IP_MULTICAST
	fprintf(stderr, "WARNING: disabled multicast\n");
#endif

	// Intialize random number generator
	random_init();

	// Compute clock frequency.
	init_clock_mhz();

	if (getenv("PBFT_J2EE") != NULL) {
		j2ee = true;
	} else {
		j2ee = false;
	}
	// Read private configuration file:
	char pk1[1024], pk2[1024];
	fscanf(config_priv, "%s %s\n", pk1, pk2);
	bigint n1(pk1, 16);
	bigint n2(pk2, 16);
	if (n1 >= n2)
		th_fail("Invalid private file: first number >= second number");

	priv_key = new rabin_priv(n1, n2);
	// TODO: this file should be encrypted under some passphrase and user
	// should be prompted for that passphrase.


	// Read public configuration file:
	// TODO: this should be more robust
	fscanf(config_file, "%256s\n", service_name);

	// read max_faulty and compute derived variables
	fscanf(config_file, "%d\n", &max_faulty);
	num_PBFT_R_replicas = 3* max_faulty + 1;
	if (num_PBFT_R_replicas > Max_num_replicas)
		th_fail("Invalid number of PBFT_R_replicas");
	threshold = num_PBFT_R_replicas - max_faulty;

	// Read authentication timeout
	int at;
	fscanf(config_file, "%d\n", &at);

	// read in all the principals
	char addPBFT_R_buff[450];
	char pk[1024];
	short port;

	fscanf(config_file, "%d\n", &num_principals);
	//  if (num_PBFT_R_replicas > num_principals)
	//  th_fail("Invalid argument");

	// read in group principal's address
	fscanf(config_file, "%256s %hd\n", addPBFT_R_buff, &port);
	Addr a;
	bzero((char*)&a, sizeof(a));
	a.sin_family = AF_INET;
	a.sin_addr.s_addr = inet_addr(addPBFT_R_buff);
	a.sin_port = htons(port);
	group = new PBFT_R_Principal(num_principals + 1, a);

	// read in remaining principals' addresses and figure out my principal
	//char host_name[MAXHOSTNAMELEN + 1];
	// if (gethostname(host_name, MAXHOSTNAMELEN))
	// {
	// perror("Unable to get hostname");
	// exit(1);
	// }

	fprintf(stderr, "PBFT_R_Node: hostname = %s\n", host_name);
	struct hostent *hent = gethostbyname(host_name);
	if (hent == 0)
		th_fail("Could not get hostent");
	struct in_addr my_address = *((in_addr*) hent->h_addr_list[0]);
	node_id = -1;

	fprintf(stderr, "allocating %d\n", num_principals
	* sizeof(PBFT_R_Principal*));
	principals = (PBFT_R_Principal**) malloc(num_principals
			* sizeof(PBFT_R_Principal*));
	for (int i = 0; i < num_principals; i++)
	{
		char hn[MAXHOSTNAMELEN+1];
		fscanf(config_file, "%256s %32s %hd %1024s \n", hn, addPBFT_R_buff,
				&port, pk);
		a.sin_addr.s_addr = inet_addr(addPBFT_R_buff);
		a.sin_port = htons(port);
		principals[i] = new PBFT_R_Principal(i, a, pk);
		if (my_address.s_addr == a.sin_addr.s_addr && node_id == -1
				&& (req_port == 0 || req_port == port))
		{
			node_id = i;
			fprintf(stderr, "PBFT_R_Node: We have parsed entry %s and assigned node_id = %d\n", hn, node_id);
		}
	}

	if (node_id < 0)
		th_fail("Could not find my principal");

	fprintf(stderr, "PBFT_R_Node: making progress\n");
	// Initialize current view number and primary.
	v = 0;
	cuPBFT_R_primary = 0;

	// Initialize memory allocator for messages.
	PBFT_R_Message::init();

	fprintf(stderr, "PBFT_R_Node: making progress 2\n");

	// Initialize socket.
	sock = socket(AF_INET, SOCK_DGRAM, 0);

	// name the socket
	Addr tmp;
	tmp.sin_family = AF_INET;
	tmp.sin_addr.s_addr = htonl(INADDR_ANY);
	tmp.sin_port = principals[node_id]->address()->sin_port;
	int error = bind(sock, (struct sockaddr*) &tmp, sizeof(Addr));
	if (error < 0)
	{
		perror("Unable to name socket");
		exit(1);
	}

#define WANMCAST
#ifdef WANMCAST
	// Set TTL larger than 1 to enable multicast across routers.
	u_char i = 20;
	error = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char *) &i,
			sizeof(i));
	if (error < 0)
	{
		perror("unable to change TTL value");
		exit(1);
	}
#endif

	fprintf(stderr, "PBFT_R_Node: making progress 3\n");

	//#define NO_UDP_CHECKSUM
#ifdef NO_UDP_CHECKSUM
	int no_check = 1;
	error = setsockopt(sock, SOL_SOCKET, SO_NO_CHECK,
			(char*)&no_check, sizeof(no_check));
	if (error < 0)
	{
		perror("unable to turn of UDP checksumming");
		exit(1);
	}
#endif //NO_UDP_CHECKSUM
#define ASYNC_SOCK
#ifdef ASYNC_SOCK
	error = fcntl(sock, F_SETFL, O_NONBLOCK);

	if (error < 0)
	{
		perror("unable to set socket to asynchronous mode");
		exit(1);
	}
#endif // ASYNC_SOCK

	// Sleep for more than a second to ensure strictly increasing
	// timestamps.

	fprintf(stderr, "PBFT_R_Node: making progress 6\n");

	sleep(2);

	// Compute new timestamp for cuPBFT_R_rid
	new_tstamp();

	last_new_key = 0;
	atimer = new PBFT_R_ITimer(at, atimePBFT_R_handler);
	fprintf(stderr, "PBFT_R_Node: making progress 7\n");

	fprintf(stderr, "PBFT_R_Node: %d %d %d %d\n", max_out, n(), num_principals, node_id);
	hidden_client_sock = -1;
}

PBFT_R_Node::~PBFT_R_Node()
{
	for (int i = 0; i < num_principals; i++)
		delete principals[i];

	free(principals);
	delete group;
}

void PBFT_R_Node::send(PBFT_R_Message *m, int i)
{
	th_assert(i == All_PBFT_R_replicas || (i >= 0 && i < num_principals),
			"Invalid argument");

#ifdef NO_IP_MULTICAST
	if (i == All_PBFT_R_replicas)
	{
		for (int x=0; x<num_PBFT_R_replicas; x++)
		send(m,x);
		return;
	}
#endif

	const Addr *to = (i == All_PBFT_R_replicas) ? group->address()
			: principals[i]->address();

	int error = 0;
	int size = m->size();
	//fprintf(stderr, "%d I need to send %d size\n", id(), size);
	while (error < size)
	{
		INCPBFT_R_OP(num_sendto);INCPBFT_R_CNT(bytes_out,size);START_CC(sendto_cycles);

		error = sendto(sock, m->contents(), size, 0, (struct sockaddr*) to,
				sizeof(Addr));

		STOP_CC(sendto_cycles);
#ifndef NDEBUG
		if (error < 0 && error != EAGAIN)
		perror("PBFT_R_Node::send: sendto");
#endif
	}
}

bool PBFT_R_Node::has_messages(long to)
{
	START_CC(handle_timeouts_cycles);
	PBFT_R_ITimer::handle_timeouts();STOP_CC(handle_timeouts_cycles);

	// PBFT_R_Timeout period for select. It puts a lower bound on the timeout
	// granularity for other timers.
	START_CC(select_cycles);
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = to;
	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(sock, &fdset);
	if (hidden_client_sock != -1)
		FD_SET(hidden_client_sock, &fdset);
	int ret = 0;
	if (j2ee && hidden_client_sock != -1)
		ret = select(hidden_client_sock+1, &fdset, 0, 0, &timeout);
	else
		ret = select(sock+1, &fdset, 0, 0, &timeout);

	if (ret > 0 && (FD_ISSET(sock, &fdset) || (j2ee && (hidden_client_sock != -1) && FD_ISSET(hidden_client_sock, &fdset))))
	{
		STOP_CC(select_cycles);INCPBFT_R_OP(select_success);
		return true;
	}STOP_CC(select_cycles);INCPBFT_R_OP(select_fail);
	return false;
}

PBFT_R_Message* PBFT_R_Node::recv()
{
	PBFT_R_Message* m = new PBFT_R_Message(Max_message_size);

	while (1)
	{
#ifndef ASYNC_SOCK
		while (!has_messages(20000));
#endif

		INCPBFT_R_OP(num_recvfrom);START_CC(recvfrom_cycles);

		//		fprintf(stderr, "\n**************\n**************\n%d recv is called...\n**************\n**************\n", id());
		int ret = recvfrom(sock, m->contents(), m->msize(), 0, 0, 0);
		//		fprintf(stderr, "%d recv returned... (ret = %d size = %d)\n", id(), ret, m->size());

		if (j2ee && ret == -1 && hidden_client_sock != -1)
		{
			ret = recvfrom(hidden_client_sock, m->contents(), m->msize(), 0, 0,
					0);
			if (ret != -1)
			{
				fprintf(stderr, "PBFT_R_Node: Receiving something from the hidden socket...\n");
				PBFT_R_Request *req;
				PBFT_R_Request::convert(m, req);

				if (req->is_read_only() || req->size()
						> PBFT_R_Request::big_req_thresh)
				{
					// read-only requests and big requests are multicast to all PBFT_R_replicas.
					//fprintf(stderr, "%d PBFT_R_Client:send_request to All_PBFT_R_replicas\n", PBFT_R_node->id());
					send(m, All_PBFT_R_replicas);
				}
			}
		}

		STOP_CC(recvfrom_cycles);

#ifdef LOOSE_MESSAGES
		if (lrand48()%100 < 4)
		{
			ret = 0;
		}
#endif

		if (ret >= (int) sizeof(PBFT_R_Message_rep) && ret >= m->size())
		{
#ifdef ASYNC_SOCK
			PBFT_R_ITimer::handle_timeouts();INCPBFT_R_OP(num_recv_success);INCPBFT_R_CNT(bytes_in,m->size());
#endif
			return m;
		}
#ifdef ASYNC_SOCK
		while (!has_messages(20000))
			;
#endif
	}
}

void PBFT_R_Node::gen_auth(char *s, unsigned l, bool in, char *dest) const
{
	INCPBFT_R_OP(num_gen_auth);START_CC(gen_auth_cycles);

#ifdef USE_SECRET_SUFFIX_MD5
	// Initialize context with "digest" of message.
	MD5_CTX context, context1;
	MD5Init(&context1);
	MD5Update(&context1, s, l);

	for (int i=0; i < num_PBFT_R_replicas; i++)
	{
		// Skip myself.
		if (i == node_id) continue;

		memcpy((char*)&context, (char*)&context1, sizeof(MD5_CTX));
		principals[i]->end_mac(&context, dest, in);
		dest += MAC_size;
	}
#else
	long long unonce = PBFT_R_Principal::new_umac_nonce();
	memcpy(dest, (char*) &unonce, UNonce_size);
	dest += UNonce_size;

	for (int i = 0; i < num_PBFT_R_replicas; i++)
	{
		// Skip myself.
		if (i == node_id)
			continue;

		if (in)
			principals[i]->gen_mac_in(s, l, dest, (char*) &unonce);
		else
			principals[i]->gen_mac_out(s, l, dest, (char*) &unonce);
		dest += UMAC_size;
	}
#endif

	STOP_CC(gen_auth_cycles);
}

bool PBFT_R_Node::verify_auth(int i, char *s, unsigned l, bool in, char *dest) const
{
	th_assert(node_id < num_PBFT_R_replicas, "Called by non-PBFT_R_replica");

	INCPBFT_R_OP(num_vePBFT_R_auth);START_CC(vePBFT_R_auth_cycles);

	PBFT_R_Principal *p = i_to_p(i);

	// PBFT_R_Principal never verifies its own authenticator.
	if (p != 0 && i != node_id)
	{

#ifdef USE_SECRET_SUFFIX_MD5
		int offset = node_id*MAC_size;
		if (node_id> i) offset -= MAC_size;
		bool ret = (in) ? p->verify_mac_in(s, l, dest+offset) : p->verify_mac_out(s, l, dest+offset);
#else
		long long unonce;
		memcpy((char*) &unonce, dest, UNonce_size);
		dest += UNonce_size;
		int offset = node_id * UMAC_size;
		if (node_id > i)
			offset -= UMAC_size;
		bool ret =
				(in) ? p->verify_mac_in(s, l, dest + offset, (char*) &unonce)
						: p->verify_mac_out(s, l, dest + offset,
								(char*) &unonce);
#endif

		STOP_CC(vePBFT_R_auth_cycles);
		return ret;
	}

	STOP_CC(vePBFT_R_auth_cycles);
	return false;
}

void PBFT_R_Node::gen_signature(const char *src, unsigned src_len, char *sig)
{
	INCPBFT_R_OP(num_sig_gen);START_CC(sig_gen_cycles);

	bigint bsig = priv_key->sign(str(src, src_len));
	int size = mpz_rawsize(&bsig);
	if (size + sizeof(unsigned) > sig_size())
		th_fail("Signature is too big");

	memcpy(sig, (char*) &size, sizeof(unsigned));
	sig += sizeof(unsigned);

	mpz_get_raw(sig, size, &bsig);

	STOP_CC(sig_gen_cycles);
}

unsigned PBFT_R_Node::decrypt(char *src, unsigned src_len, char *dst,
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

Request_id PBFT_R_Node::new_rid()
{
	if ((unsigned) cuPBFT_R_rid == (unsigned) 0xffffffff)
	{
		new_tstamp();
	}
	return ++cuPBFT_R_rid;
}

void PBFT_R_Node::new_tstamp()
{
	struct timeval t;
	gettimeofday(&t, 0);
	th_assert(sizeof(t.tv_sec) <= sizeof(int), "tv_sec is too big");
	Long tstamp = t.tv_sec;
	long int_bits = sizeof(int) * 8;
	cuPBFT_R_rid = tstamp << int_bits;
}

void atimePBFT_R_handler()
{
	th_assert(PBFT_R_node, "PBFT_R_replica is not initialized\n");

	// Multicast new key to all PBFT_R_replicas.
	PBFT_R_node->send_new_key();
}

void PBFT_R_Node::send_new_key()
{
	delete last_new_key;

	// Multicast new key to all PBFT_R_replicas.
	last_new_key = new PBFT_R_New_key();
	send(last_new_key, All_PBFT_R_replicas);

	// Stop timer if not expired and then restart it
	atimer->stop();
	atimer->restart();
}

