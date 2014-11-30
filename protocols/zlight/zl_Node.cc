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
#include "zl_Message.h"
#include "zl_Principal.h"
#include "zl_Node.h"

#include "zl_Time.h"
#include "zl_ITimer.h"

// Pointer to global node instance.
zl_Node *zl_node = 0;
zl_Principal *zl_my_principal = 0;

zl_Node::zl_Node(FILE *config_file, FILE *config_priv, char *host_name, short req_port)
{
	zl_node = this;

	init_clock_mhz();

	fprintf(stderr, "zl_Node: clock_mhz is %llu\n", clock_mhz);

	// Read public configuration file:
	// read max_faulty and compute derived variables
	fscanf(config_file, "%d\n", &max_faulty);
	num_replicas = 3 * max_faulty + 1;
	if (num_replicas > Max_num_replicas)
	{
		th_fail("Invalid number of replicas");
	}

	// read in all the principals
	char addr_buff[100];
	char pk[1024];
	short port;

	fscanf(config_file, "%d\n", &num_principals);
	if (num_replicas > num_principals)
	{
		th_fail("Invalid argument");
	}

	// Initialize primary
	cur_primary = 0;

	// read in group principal's address
	fscanf(config_file, "%256s %hd\n", addr_buff, &port);
	Addr a;
	bzero((char*)&a, sizeof(a));
	a.sin_family = AF_INET;
	a.sin_addr.s_addr = inet_addr(addr_buff);
	a.sin_port = htons(port);
	group = new zl_Principal(num_principals+1, num_principals, a, NULL);

	// read in remaining principals' addresses and figure out my principal
	//char host_name[MAXHOSTNAMELEN+1];
	//if (gethostname(host_name, MAXHOSTNAMELEN))
	//{
	//	perror("Unable to get hostname");
	//	exit(1);
	//}

	struct hostent *hent = gethostbyname(host_name);
	if (hent == 0)
	{
		th_fail("Could not get hostent");
	}

	struct in_addr my_address = *((in_addr*)hent->h_addr_list[0]);
	node_id = -1;

	principals = (zl_Principal**)malloc(num_principals*sizeof(zl_Principal*));

	fprintf(stderr, "num_principals = %d\n", num_principals);
	char temp_host_name[MAXHOSTNAMELEN+1];
	for (int i=0; i < num_principals; i++)
	{
		fscanf(config_file, "%256s %32s %hd %1024s \n", temp_host_name,
				addr_buff, &port, pk);

		a.sin_addr.s_addr = inet_addr(addr_buff);
		a.sin_port = htons(port);

		principals[i] = new zl_Principal(i, num_principals, a, pk);
		//fprintf(stderr, "We have parsed entry %s\n", temp_host_name);

		if (my_address.s_addr == a.sin_addr.s_addr && node_id == -1
				&& (req_port == 0 || req_port == port))
		{
			node_id = i;
			zl_my_principal = principals[i];
			fprintf(stderr, "We have parsed entry %s and assigned node_id = %d\n", temp_host_name, node_id);
		}
	}

	if (node_id < 0)
	{
		th_fail("Could not find my principal");
	}

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
	    fprintf(stderr, "zl_Node: Will not init priv_key\n");
	    priv_key = NULL;
	}

	// Initialize memory allocator for messages.
//	zl_Message::init();

	// Initialize socket.
	sock = socket(AF_INET, SOCK_DGRAM, 0);

	// name the socket
	Addr tmp;
	tmp.sin_family = AF_INET;
	tmp.sin_addr.s_addr = htonl(INADDR_ANY);
	tmp.sin_port = principals[node_id]->address()->sin_port;
	int error = bind(sock, (struct sockaddr*)&tmp, sizeof(Addr));
	if (error < 0)
	{
		perror("Unable to name socket");
		exit(1);
	}

	// Set TTL larger than 1 to enable multicast across routers.
	u_char i = 20;
	error = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&i,
			sizeof(i));
	if (error < 0)
	{
		perror("unable to change TTL value");
		exit(1);
	}

	error = fcntl(sock, F_SETFL, O_NONBLOCK);
	if (error < 0)
	{
		perror("unable to set socket to asynchronous mode");
		exit(1);
	}

	// Sleep for more than a second to ensure strictly increasing
	// timestamps.
	sleep(2);

	// Compute new timestamp for cur_rid
	new_tstamp();
}

zl_Node::~zl_Node()
{
	for (int i=0; i < num_principals; i++)
	{
		delete principals[i];
	}

	free(principals);
	delete group;
}

void zl_Node::send(zl_Message *m, int i)
{
	th_assert(i == zl_All_replicas || (i >= 0&& i < num_principals),
			"Invalid argument");

	const Addr *to =(i == zl_All_replicas) ? group->address()
			: principals[i]->address();

	int error = 0;
	while (error < (int)m->size())
	{
		error = sendto(sock, m->contents(), m->size(), 0, (struct sockaddr*)to,
				sizeof(Addr));
	}
}

bool zl_Node::has_messages(long to)
{
	zl_ITimer::handle_timeouts();
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = to;
	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(sock, &fdset);
	int ret = select(sock+1, &fdset, 0, 0, &timeout);
	if (ret > 0&& FD_ISSET(sock, &fdset))
	{
		return true;
	}
	return false;
}

zl_Message* zl_Node::recv()
{
	zl_Message* m= new zl_Message(zl_Max_message_size);
	while (1)
	{
#ifdef TRACES
		fprintf(stderr, "zl_Node[%d]::recv(): reading the socket\n", id());
#endif
		int ret = recvfrom(sock, m->contents(), m->msize(), 0, 0, 0);
		if (ret >= (int)sizeof(zl_Message_rep) && ret >= (int)m->size())
		{
#ifdef TRACES
			fprintf(stderr, "zl_Node[%d]::recv(): one message received on the socket\n", id());
#endif
			zl_ITimer::handle_timeouts();
			return m;
		}
		while (!has_messages(20000))
			;
	}
}

void zl_Node::gen_auth(char *s, unsigned l, char *dest) const
{
	long long unonce = zl_Principal::new_umac_nonce();
	memcpy(dest, (char*)&unonce, zl_UNonce_size);
	dest += zl_UNonce_size;

	zl_Principal *p =i_to_p(id());

	for (int i=0; i < num_replicas; i++)
	{
		// node never authenticates message to itself
		if (i == id())
		    continue;
		p->gen_mac(s, l, dest, i, (char*)&unonce);
		dest += zl_UMAC_size;
	}
}

bool zl_Node::verify_auth(int src_pid, char *s, unsigned l, char *dest) const
{
	th_assert(node_id < num_replicas, "Called by non-replica");

	zl_Principal *p = i_to_p(src_pid);

	// Principal never verifies its own authenticator.
	if (p != 0 && src_pid != node_id)
	{
		long long unonce;
		memcpy((char*)&unonce, dest, zl_UNonce_size);
		dest += zl_UNonce_size;
		int offset = node_id*zl_UMAC_size;
		if (node_id > src_pid)
		{
			offset -= zl_UMAC_size;
		}

		return p->verify_mac(s, l, dest + offset, (char*)&unonce);
	}
	return false;
}

void zl_Node::gen_signature(const char *src, unsigned src_len, char *sig)
{
	bigint bsig = priv_key->sign(str(src, src_len));
	int size = mpz_rawsize(&bsig);
	if (size + sizeof(unsigned) > sig_size())
		th_fail("Signature is too big");

	memcpy(sig, (char*) &size, sizeof(unsigned));
	sig += sizeof(unsigned);

	mpz_get_raw(sig, size, &bsig);
}

unsigned zl_Node::decrypt(char *src, unsigned src_len, char *dst,
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


Request_id zl_Node::new_rid()
{
	if ((unsigned)cur_rid == (unsigned)0xffffffff)
	{
		new_tstamp();
	}
	return ++cur_rid;
}

void zl_Node::new_tstamp()
{
	struct timeval t;
	gettimeofday(&t, 0);
	th_assert(sizeof(t.tv_sec) <= sizeof(int), "tv_sec is too big");
	Long tstamp = t.tv_sec;
	long int_bits = sizeof(int)*8;
	cur_rid = tstamp << int_bits;
}
